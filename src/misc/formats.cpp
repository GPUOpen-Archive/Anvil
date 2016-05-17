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

#include "misc/debug.h"
#include "misc/formats.h"

static struct FormatInfo
{
    VkFormat               format;
    Anvil::ComponentLayout component_layout;
    uint8_t                component_bits[4]; /* if [0] == [1] == [2] == [3] == 0, the format is a block format. */
    Anvil::FormatType      format_type;
} formats[] =
{
    /* format                             | component_layout                | component_bits[0] | component_bits[1] | component_bits[2] | component_bits[3] | format_type */
    {VK_FORMAT_UNDEFINED,                   Anvil::COMPONENT_LAYOUT_UNKNOWN,  0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNKNOWN},
    {VK_FORMAT_R4G4_UNORM_PACK8,            Anvil::COMPONENT_LAYOUT_RG,       4,                  4,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R4G4B4A4_UNORM_PACK16,       Anvil::COMPONENT_LAYOUT_RGBA,     4,                  4,                  4,                  4,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_B4G4R4A4_UNORM_PACK16,       Anvil::COMPONENT_LAYOUT_BGRA,     4,                  4,                  4,                  4,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R5G6B5_UNORM_PACK16,         Anvil::COMPONENT_LAYOUT_RGB,      5,                  6,                  5,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_B5G6R5_UNORM_PACK16,         Anvil::COMPONENT_LAYOUT_BGR,      5,                  6,                  5,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R5G5B5A1_UNORM_PACK16,       Anvil::COMPONENT_LAYOUT_RGBA,     5,                  5,                  5,                  1,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_B5G5R5A1_UNORM_PACK16,       Anvil::COMPONENT_LAYOUT_BGRA,     5,                  5,                  5,                  1,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_A1R5G5B5_UNORM_PACK16,       Anvil::COMPONENT_LAYOUT_ARGB,     1,                  5,                  5,                  5,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R8_UNORM,                    Anvil::COMPONENT_LAYOUT_R,        8,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R8_SNORM,                    Anvil::COMPONENT_LAYOUT_R,        8,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_R8_USCALED,                  Anvil::COMPONENT_LAYOUT_R,        8,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_R8_SSCALED,                  Anvil::COMPONENT_LAYOUT_R,        8,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_R8_UINT,                     Anvil::COMPONENT_LAYOUT_R,        8,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R8_SINT,                     Anvil::COMPONENT_LAYOUT_R,        8,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R8_SRGB,                     Anvil::COMPONENT_LAYOUT_R,        8,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_R8G8_UNORM,                  Anvil::COMPONENT_LAYOUT_RG,       8,                  8,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R8G8_SNORM,                  Anvil::COMPONENT_LAYOUT_RG,       8,                  8,                  0,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_R8G8_USCALED,                Anvil::COMPONENT_LAYOUT_RG,       8,                  8,                  0,                  0,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_R8G8_SSCALED,                Anvil::COMPONENT_LAYOUT_RG,       8,                  8,                  0,                  0,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_R8G8_UINT,                   Anvil::COMPONENT_LAYOUT_RG,       8,                  8,                  0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R8G8_SINT,                   Anvil::COMPONENT_LAYOUT_RG,       8,                  8,                  0,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R8G8_SRGB,                   Anvil::COMPONENT_LAYOUT_RG,       8,                  8,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_R8G8B8_UNORM,                Anvil::COMPONENT_LAYOUT_RGB,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R8G8B8_SNORM,                Anvil::COMPONENT_LAYOUT_RGB,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_R8G8B8_USCALED,              Anvil::COMPONENT_LAYOUT_RGB,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_R8G8B8_SSCALED,              Anvil::COMPONENT_LAYOUT_RGB,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_R8G8B8_UINT,                 Anvil::COMPONENT_LAYOUT_RGB,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R8G8B8_SINT,                 Anvil::COMPONENT_LAYOUT_RGB,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R8G8B8_SRGB,                 Anvil::COMPONENT_LAYOUT_RGB,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_B8G8R8_UNORM,                Anvil::COMPONENT_LAYOUT_BGR,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_B8G8R8_SNORM,                Anvil::COMPONENT_LAYOUT_BGR,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_B8G8R8_USCALED,              Anvil::COMPONENT_LAYOUT_BGR,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_B8G8R8_SSCALED,              Anvil::COMPONENT_LAYOUT_BGR,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_B8G8R8_UINT,                 Anvil::COMPONENT_LAYOUT_BGR,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_B8G8R8_SINT,                 Anvil::COMPONENT_LAYOUT_BGR,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_B8G8R8_SRGB,                 Anvil::COMPONENT_LAYOUT_BGR,      8,                  8,                  8,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_R8G8B8A8_UNORM,              Anvil::COMPONENT_LAYOUT_RGBA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R8G8B8A8_SNORM,              Anvil::COMPONENT_LAYOUT_RGBA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_R8G8B8A8_USCALED,            Anvil::COMPONENT_LAYOUT_RGBA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_R8G8B8A8_SSCALED,            Anvil::COMPONENT_LAYOUT_RGBA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_R8G8B8A8_UINT,               Anvil::COMPONENT_LAYOUT_RGBA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R8G8B8A8_SINT,               Anvil::COMPONENT_LAYOUT_RGBA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R8G8B8A8_SRGB,               Anvil::COMPONENT_LAYOUT_RGBA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_B8G8R8A8_UNORM,              Anvil::COMPONENT_LAYOUT_BGRA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_B8G8R8A8_SNORM,              Anvil::COMPONENT_LAYOUT_BGRA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_B8G8R8A8_USCALED,            Anvil::COMPONENT_LAYOUT_BGRA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_B8G8R8A8_SSCALED,            Anvil::COMPONENT_LAYOUT_BGRA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_B8G8R8A8_UINT,               Anvil::COMPONENT_LAYOUT_BGRA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_B8G8R8A8_SINT,               Anvil::COMPONENT_LAYOUT_BGRA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_B8G8R8A8_SRGB,               Anvil::COMPONENT_LAYOUT_BGRA,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_A8B8G8R8_UNORM_PACK32,       Anvil::COMPONENT_LAYOUT_ABGR,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_A8B8G8R8_SNORM_PACK32,       Anvil::COMPONENT_LAYOUT_ABGR,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_A8B8G8R8_USCALED_PACK32,     Anvil::COMPONENT_LAYOUT_ABGR,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_A8B8G8R8_SSCALED_PACK32,     Anvil::COMPONENT_LAYOUT_ABGR,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_A8B8G8R8_UINT_PACK32,        Anvil::COMPONENT_LAYOUT_ABGR,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_A8B8G8R8_SINT_PACK32,        Anvil::COMPONENT_LAYOUT_ABGR,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_A8B8G8R8_SRGB_PACK32,        Anvil::COMPONENT_LAYOUT_ABGR,     8,                  8,                  8,                  8,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_A2R10G10B10_UNORM_PACK32,    Anvil::COMPONENT_LAYOUT_ARGB,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_A2R10G10B10_SNORM_PACK32,    Anvil::COMPONENT_LAYOUT_ARGB,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_A2R10G10B10_USCALED_PACK32,  Anvil::COMPONENT_LAYOUT_ARGB,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_A2R10G10B10_SSCALED_PACK32,  Anvil::COMPONENT_LAYOUT_ARGB,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_A2R10G10B10_UINT_PACK32,     Anvil::COMPONENT_LAYOUT_ARGB,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_A2R10G10B10_SINT_PACK32,     Anvil::COMPONENT_LAYOUT_ARGB,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_A2B10G10R10_UNORM_PACK32,    Anvil::COMPONENT_LAYOUT_ABGR,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_A2B10G10R10_SNORM_PACK32,    Anvil::COMPONENT_LAYOUT_ABGR,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_A2B10G10R10_USCALED_PACK32,  Anvil::COMPONENT_LAYOUT_ABGR,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_A2B10G10R10_SSCALED_PACK32,  Anvil::COMPONENT_LAYOUT_ABGR,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_A2B10G10R10_UINT_PACK32,     Anvil::COMPONENT_LAYOUT_ABGR,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_A2B10G10R10_SINT_PACK32,     Anvil::COMPONENT_LAYOUT_ABGR,     2,                  10,                 10,                 10,                 Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R16_UNORM,                   Anvil::COMPONENT_LAYOUT_R,        16,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R16_SNORM,                   Anvil::COMPONENT_LAYOUT_R,        16,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_R16_USCALED,                 Anvil::COMPONENT_LAYOUT_R,        16,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_R16_SSCALED,                 Anvil::COMPONENT_LAYOUT_R,        16,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_R16_UINT,                    Anvil::COMPONENT_LAYOUT_R,        16,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R16_SINT,                    Anvil::COMPONENT_LAYOUT_R,        16,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R16_SFLOAT,                  Anvil::COMPONENT_LAYOUT_R,        16,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R16G16_UNORM,                Anvil::COMPONENT_LAYOUT_RG,       16,                 16,                 0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R16G16_SNORM,                Anvil::COMPONENT_LAYOUT_RG,       16,                 16,                 0,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_R16G16_USCALED,              Anvil::COMPONENT_LAYOUT_RG,       16,                 16,                 0,                  0,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_R16G16_SSCALED,              Anvil::COMPONENT_LAYOUT_RG,       16,                 16,                 0,                  0,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_R16G16_UINT,                 Anvil::COMPONENT_LAYOUT_RG,       16,                 16,                 0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R16G16_SINT,                 Anvil::COMPONENT_LAYOUT_RG,       16,                 16,                 0,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R16G16_SFLOAT,               Anvil::COMPONENT_LAYOUT_RG,       16,                 16,                 0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R16G16B16_UNORM,             Anvil::COMPONENT_LAYOUT_RGB,      16,                 16,                 16,                 0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R16G16B16_SNORM,             Anvil::COMPONENT_LAYOUT_RGB,      16,                 16,                 16,                 0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_R16G16B16_USCALED,           Anvil::COMPONENT_LAYOUT_RGB,      16,                 16,                 16,                 0,                  Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_R16G16B16_SSCALED,           Anvil::COMPONENT_LAYOUT_RGB,      16,                 16,                 16,                 0,                  Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_R16G16B16_UINT,              Anvil::COMPONENT_LAYOUT_RGB,      16,                 16,                 16,                 0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R16G16B16_SINT,              Anvil::COMPONENT_LAYOUT_RGB,      16,                 16,                 16,                 0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R16G16B16_SFLOAT,            Anvil::COMPONENT_LAYOUT_RGB,      16,                 16,                 16,                 0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R16G16B16A16_UNORM,          Anvil::COMPONENT_LAYOUT_RGBA,     16,                 16,                 16,                 16,                 Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_R16G16B16A16_SNORM,          Anvil::COMPONENT_LAYOUT_RGBA,     16,                 16,                 16,                 16,                 Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_R16G16B16A16_USCALED,        Anvil::COMPONENT_LAYOUT_RGBA,     16,                 16,                 16,                 16,                 Anvil::FORMAT_TYPE_USCALED},
    {VK_FORMAT_R16G16B16A16_SSCALED,        Anvil::COMPONENT_LAYOUT_RGBA,     16,                 16,                 16,                 16,                 Anvil::FORMAT_TYPE_SSCALED},
    {VK_FORMAT_R16G16B16A16_UINT,           Anvil::COMPONENT_LAYOUT_RGBA,     16,                 16,                 16,                 16,                 Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R16G16B16A16_SINT,           Anvil::COMPONENT_LAYOUT_RGBA,     16,                 16,                 16,                 16,                 Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R16G16B16A16_SFLOAT,         Anvil::COMPONENT_LAYOUT_RGBA,     16,                 16,                 16,                 16,                 Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R32_UINT,                    Anvil::COMPONENT_LAYOUT_R,        32,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R32_SINT,                    Anvil::COMPONENT_LAYOUT_R,        32,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R32_SFLOAT,                  Anvil::COMPONENT_LAYOUT_R,        32,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R32G32_UINT,                 Anvil::COMPONENT_LAYOUT_RG,       32,                 32,                 0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R32G32_SINT,                 Anvil::COMPONENT_LAYOUT_RG,       32,                 32,                 0,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R32G32_SFLOAT,               Anvil::COMPONENT_LAYOUT_RG,       32,                 32,                 0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R32G32B32_UINT,              Anvil::COMPONENT_LAYOUT_RGB,      32,                 32,                 32,                 0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R32G32B32_SINT,              Anvil::COMPONENT_LAYOUT_RGB,      32,                 32,                 32,                 0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R32G32B32_SFLOAT,            Anvil::COMPONENT_LAYOUT_RGB,      32,                 32,                 32,                 0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R32G32B32A32_UINT,           Anvil::COMPONENT_LAYOUT_RGBA,     32,                 32,                 32,                 32,                 Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R32G32B32A32_SINT,           Anvil::COMPONENT_LAYOUT_RGBA,     32,                 32,                 32,                 32,                 Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R32G32B32A32_SFLOAT,         Anvil::COMPONENT_LAYOUT_RGBA,     32,                 32,                 32,                 32,                 Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R64_UINT,                    Anvil::COMPONENT_LAYOUT_R,        64,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R64_SINT,                    Anvil::COMPONENT_LAYOUT_R,        64,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R64_SFLOAT,                  Anvil::COMPONENT_LAYOUT_R,        64,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R64G64_UINT,                 Anvil::COMPONENT_LAYOUT_RG,       64,                 64,                 0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R64G64_SINT,                 Anvil::COMPONENT_LAYOUT_RG,       64,                 64,                 0,                  0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R64G64_SFLOAT,               Anvil::COMPONENT_LAYOUT_RG,       64,                 64,                 0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R64G64B64_UINT,              Anvil::COMPONENT_LAYOUT_RGB,      64,                 64,                 64,                 0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R64G64B64_SINT,              Anvil::COMPONENT_LAYOUT_RGB,      64,                 64,                 64,                 0,                  Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R64G64B64_SFLOAT,            Anvil::COMPONENT_LAYOUT_RGB,      64,                 64,                 64,                 0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_R64G64B64A64_UINT,           Anvil::COMPONENT_LAYOUT_RGBA,     64,                 64,                 64,                 64,                 Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_R64G64B64A64_SINT,           Anvil::COMPONENT_LAYOUT_RGBA,     64,                 64,                 64,                 64,                 Anvil::FORMAT_TYPE_SINT},
    {VK_FORMAT_R64G64B64A64_SFLOAT,         Anvil::COMPONENT_LAYOUT_RGBA,     64,                 64,                 64,                 64,                 Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_B10G11R11_UFLOAT_PACK32,     Anvil::COMPONENT_LAYOUT_BGR,      10,                 11,                 11,                 0,                  Anvil::FORMAT_TYPE_UFLOAT},
    {VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,      Anvil::COMPONENT_LAYOUT_EBGR,     5,                  9,                  9,                  9,                  Anvil::FORMAT_TYPE_UFLOAT},
    {VK_FORMAT_D16_UNORM,                   Anvil::COMPONENT_LAYOUT_D,        16,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_X8_D24_UNORM_PACK32,         Anvil::COMPONENT_LAYOUT_XD,       8,                  24,                 0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_D32_SFLOAT,                  Anvil::COMPONENT_LAYOUT_DS,       32,                 0,                  0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_S8_UINT,                     Anvil::COMPONENT_LAYOUT_S,        8,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UINT},
    {VK_FORMAT_D16_UNORM_S8_UINT,           Anvil::COMPONENT_LAYOUT_DS,       16,                 8,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM_UINT},
    {VK_FORMAT_D24_UNORM_S8_UINT,           Anvil::COMPONENT_LAYOUT_DS,       24,                 8,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM_UINT},
    {VK_FORMAT_D32_SFLOAT_S8_UINT,          Anvil::COMPONENT_LAYOUT_DS,       32,                 8,                  0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT_UINT},
    {VK_FORMAT_BC1_RGB_UNORM_BLOCK,         Anvil::COMPONENT_LAYOUT_RGB,      0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_BC1_RGB_SRGB_BLOCK,          Anvil::COMPONENT_LAYOUT_RGB,      0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_BC1_RGBA_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_BC1_RGBA_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_BC2_UNORM_BLOCK,             Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_BC2_SRGB_BLOCK,              Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_BC3_UNORM_BLOCK,             Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_BC3_SRGB_BLOCK,              Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_BC4_UNORM_BLOCK,             Anvil::COMPONENT_LAYOUT_R,        0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_BC4_SNORM_BLOCK,             Anvil::COMPONENT_LAYOUT_R,        0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_BC5_UNORM_BLOCK,             Anvil::COMPONENT_LAYOUT_RG,       0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_BC5_SNORM_BLOCK,             Anvil::COMPONENT_LAYOUT_RG,       0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_BC6H_UFLOAT_BLOCK,           Anvil::COMPONENT_LAYOUT_RGB,      0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UFLOAT},
    {VK_FORMAT_BC6H_SFLOAT_BLOCK,           Anvil::COMPONENT_LAYOUT_RGB,      0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SFLOAT},
    {VK_FORMAT_BC7_UNORM_BLOCK,             Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_BC7_SRGB_BLOCK,              Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,     Anvil::COMPONENT_LAYOUT_RGB,      0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,      Anvil::COMPONENT_LAYOUT_RGB,      0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,   Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,    Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,   Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,    Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_EAC_R11_UNORM_BLOCK,         Anvil::COMPONENT_LAYOUT_R,        0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_EAC_R11_SNORM_BLOCK,         Anvil::COMPONENT_LAYOUT_R,        0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_EAC_R11G11_UNORM_BLOCK,      Anvil::COMPONENT_LAYOUT_RG,       0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_EAC_R11G11_SNORM_BLOCK,      Anvil::COMPONENT_LAYOUT_RG,       0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SNORM},
    {VK_FORMAT_ASTC_4x4_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_4x4_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_5x4_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_5x4_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_5x5_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_5x5_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_6x5_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_6x5_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_6x6_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_6x6_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_8x5_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_8x5_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_8x6_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_8x6_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_8x8_UNORM_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_8x8_SRGB_BLOCK,         Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_10x5_UNORM_BLOCK,       Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_10x5_SRGB_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_10x6_UNORM_BLOCK,       Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_10x6_SRGB_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_10x8_UNORM_BLOCK,       Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_10x8_SRGB_BLOCK,        Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_10x10_UNORM_BLOCK,      Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_10x10_SRGB_BLOCK,       Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_12x10_UNORM_BLOCK,      Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_12x10_SRGB_BLOCK,       Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB},
    {VK_FORMAT_ASTC_12x12_UNORM_BLOCK,      Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_UNORM},
    {VK_FORMAT_ASTC_12x12_SRGB_BLOCK,       Anvil::COMPONENT_LAYOUT_RGBA,     0,                  0,                  0,                  0,                  Anvil::FORMAT_TYPE_SRGB}
};

static uint32_t layout_to_n_components[] =
{
    /* COMPONENT_LAYOUT_ABGR */
    4,

    /* COMPONENT_LAYOUT_ARGB */
    4,

    /* COMPONENT_LAYOUT_BGR */
    3,

    /* COMPONENT_LAYOUT_BGRA */
    4,

    /* COMPONENT_LAYOUT_D */
    1,

    /* COMPONENT_LAYOUT_DS */
    2,

    /* COMPONENT_LAYOUT_EBGR */
    4,

    /* COMPONENT_LAYOUT_R */
    1,

    /* COMPONENT_LAYOUT_RG */
    2,

    /* COMPONENT_LAYOUT_RGB */
    3,

    /* COMPONENT_LAYOUT_RGBA */
    4,

    /* COMPONENT_LAYOUT_S */
    1,

    /* COMPONENT_LAYOUT_XD */
    2,
};

/** Please see header for specification */
VkFormat Anvil::Formats::get_format(ComponentLayout component_layout,
                                    FormatType      format_type,
                                    uint32_t        n_component0_bits,
                                    uint32_t        n_component1_bits,
                                    uint32_t        n_component2_bits,
                                    uint32_t        n_component3_bits)
{
    static const uint32_t n_available_formats = sizeof(formats) / sizeof(formats[0]);
    VkFormat              result              = VK_FORMAT_UNDEFINED;

    for (uint32_t n_format = 0;
                  n_format < n_available_formats;
                ++n_format)
    {
        const FormatInfo& current_format_info = formats[n_format];

        if (current_format_info.component_layout  == component_layout  &&
            current_format_info.format_type       == format_type       &&
            current_format_info.component_bits[0] == n_component0_bits &&
            current_format_info.component_bits[1] == n_component1_bits &&
            current_format_info.component_bits[2] == n_component2_bits &&
            current_format_info.component_bits[3] == n_component3_bits)
        {
            result = current_format_info.format;

            break;
        }
    }

    return result;
}

/** Please see header for specification */
Anvil::ComponentLayout Anvil::Formats::get_format_component_layout(VkFormat format)
{
    static_assert(sizeof(formats) / sizeof(formats[0]) == VK_FORMAT_RANGE_SIZE, "");

    anvil_assert(format < VK_FORMAT_RANGE_SIZE);

    return formats[format].component_layout;
}

/** Please see header for specification */
uint32_t Anvil::Formats::get_format_n_components(VkFormat format)
{
    anvil_assert(format < VK_FORMAT_RANGE_SIZE);

    return layout_to_n_components[formats[format].component_layout];
}

/** Please see header for specification */
void Anvil::Formats::get_format_n_component_bits(VkFormat  format,
                                                 uint32_t* out_channel0_bits_ptr,
                                                 uint32_t* out_channel1_bits_ptr,
                                                 uint32_t* out_channel2_bits_ptr,
                                                 uint32_t* out_channel3_bits_ptr)
{
    FormatInfo* format_props_ptr = nullptr;

    anvil_assert(format < VK_FORMAT_RANGE_SIZE);

    format_props_ptr = formats + format;

    *out_channel0_bits_ptr = format_props_ptr->component_bits[0];
    *out_channel1_bits_ptr = format_props_ptr->component_bits[1];
    *out_channel2_bits_ptr = format_props_ptr->component_bits[2];
    *out_channel3_bits_ptr = format_props_ptr->component_bits[3];
}

/** Please see header for specification */
Anvil::FormatType Anvil::Formats::get_format_type(VkFormat format)
{
    anvil_assert(format < VK_FORMAT_RANGE_SIZE);

    return formats[format].format_type;
}

/** Please see header for specification */
bool Anvil::Formats::is_format_compressed(VkFormat format)
{
    anvil_assert(format < VK_FORMAT_RANGE_SIZE);

    return (formats[format].component_bits[0] == formats[format].component_bits[1]) &&
           (formats[format].component_bits[1] == formats[format].component_bits[2]) &&
           (formats[format].component_bits[2] == formats[format].component_bits[3]) &&
           (formats[format].component_bits[0] == 0);
}
