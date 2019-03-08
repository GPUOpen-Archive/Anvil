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

#include "misc/debug.h"
#include "misc/formats.h"
#include "misc/types.h"
#include <algorithm>
#include <unordered_map>

static const struct FormatInfo
{
    Anvil::Format          format;
    const char*            name;
    Anvil::ComponentLayout component_layout;
    uint8_t                component_bits[4]; /* if [0] == [1] == [2] == [3] == 0, the format is a block format. */
    Anvil::FormatType      format_type;
    bool                   is_packed;
} g_formats[] =
{
    /* format                                  | name                                  | component_layout               | component_bits[0] | component_bits[1] | component_bits[2] | component_bits[3] | format_type                    | is_packed? */
    {Anvil::Format::UNKNOWN,                     "VK_FORMAT_UNDEFINED",                  Anvil::ComponentLayout::UNKNOWN,  0,                  0,                  0,                  0,                  Anvil::FormatType::UNKNOWN,     false},
    {Anvil::Format::R4G4_UNORM_PACK8,            "VK_FORMAT_R4G4_UNORM_PACK8",           Anvil::ComponentLayout::RG,       4,                  4,                  0,                  0,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::R4G4B4A4_UNORM_PACK16,       "VK_FORMAT_R4G4B4A4_UNORM_PACK16",      Anvil::ComponentLayout::RGBA,     4,                  4,                  4,                  4,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::B4G4R4A4_UNORM_PACK16,       "VK_FORMAT_B4G4R4A4_UNORM_PACK16",      Anvil::ComponentLayout::BGRA,     4,                  4,                  4,                  4,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::R5G6B5_UNORM_PACK16,         "VK_FORMAT_R5G6B5_UNORM_PACK16",        Anvil::ComponentLayout::RGB,      5,                  6,                  5,                  0,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::B5G6R5_UNORM_PACK16,         "VK_FORMAT_B5G6R5_UNORM_PACK16",        Anvil::ComponentLayout::BGR,      5,                  6,                  5,                  0,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::R5G5B5A1_UNORM_PACK16,       "VK_FORMAT_R5G5B5A1_UNORM_PACK16",      Anvil::ComponentLayout::RGBA,     5,                  5,                  5,                  1,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::B5G5R5A1_UNORM_PACK16,       "VK_FORMAT_B5G5R5A1_UNORM_PACK16",      Anvil::ComponentLayout::BGRA,     5,                  5,                  5,                  1,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::A1R5G5B5_UNORM_PACK16,       "VK_FORMAT_A1R5G5B5_UNORM_PACK16",      Anvil::ComponentLayout::ARGB,     1,                  5,                  5,                  5,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::R8_UNORM,                    "VK_FORMAT_R8_UNORM",                   Anvil::ComponentLayout::R,        8,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::R8_SNORM,                    "VK_FORMAT_R8_SNORM",                   Anvil::ComponentLayout::R,        8,                  0,                  0,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::R8_USCALED,                  "VK_FORMAT_R8_USCALED",                 Anvil::ComponentLayout::R,        8,                  0,                  0,                  0,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::R8_SSCALED,                  "VK_FORMAT_R8_SSCALED",                 Anvil::ComponentLayout::R,        8,                  0,                  0,                  0,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::R8_UINT,                     "VK_FORMAT_R8_UINT",                    Anvil::ComponentLayout::R,        8,                  0,                  0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R8_SINT,                     "VK_FORMAT_R8_SINT",                    Anvil::ComponentLayout::R,        8,                  0,                  0,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R8_SRGB,                     "VK_FORMAT_R8_SRGB",                    Anvil::ComponentLayout::R,        8,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::R8G8_UNORM,                  "VK_FORMAT_R8G8_UNORM",                 Anvil::ComponentLayout::RG,       8,                  8,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::R8G8_SNORM,                  "VK_FORMAT_R8G8_SNORM",                 Anvil::ComponentLayout::RG,       8,                  8,                  0,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::R8G8_USCALED,                "VK_FORMAT_R8G8_USCALED",               Anvil::ComponentLayout::RG,       8,                  8,                  0,                  0,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::R8G8_SSCALED,                "VK_FORMAT_R8G8_SSCALED",               Anvil::ComponentLayout::RG,       8,                  8,                  0,                  0,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::R8G8_UINT,                   "VK_FORMAT_R8G8_UINT",                  Anvil::ComponentLayout::RG,       8,                  8,                  0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R8G8_SINT,                   "VK_FORMAT_R8G8_SINT",                  Anvil::ComponentLayout::RG,       8,                  8,                  0,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R8G8_SRGB,                   "VK_FORMAT_R8G8_SRGB",                  Anvil::ComponentLayout::RG,       8,                  8,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::R8G8B8_UNORM,                "VK_FORMAT_R8G8B8_UNORM",               Anvil::ComponentLayout::RGB,      8,                  8,                  8,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::R8G8B8_SNORM,                "VK_FORMAT_R8G8B8_SNORM",               Anvil::ComponentLayout::RGB,      8,                  8,                  8,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::R8G8B8_USCALED,              "VK_FORMAT_R8G8B8_USCALED",             Anvil::ComponentLayout::RGB,      8,                  8,                  8,                  0,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::R8G8B8_SSCALED,              "VK_FORMAT_R8G8B8_SSCALED",             Anvil::ComponentLayout::RGB,      8,                  8,                  8,                  0,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::R8G8B8_UINT,                 "VK_FORMAT_R8G8B8_UINT",                Anvil::ComponentLayout::RGB,      8,                  8,                  8,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R8G8B8_SINT,                 "VK_FORMAT_R8G8B8_SINT",                Anvil::ComponentLayout::RGB,      8,                  8,                  8,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R8G8B8_SRGB,                 "VK_FORMAT_R8G8B8_SRGB",                Anvil::ComponentLayout::RGB,      8,                  8,                  8,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::B8G8R8_UNORM,                "VK_FORMAT_B8G8R8_UNORM",               Anvil::ComponentLayout::BGR,      8,                  8,                  8,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::B8G8R8_SNORM,                "VK_FORMAT_B8G8R8_SNORM",               Anvil::ComponentLayout::BGR,      8,                  8,                  8,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::B8G8R8_USCALED,              "VK_FORMAT_B8G8R8_USCALED",             Anvil::ComponentLayout::BGR,      8,                  8,                  8,                  0,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::B8G8R8_SSCALED,              "VK_FORMAT_B8G8R8_SSCALED",             Anvil::ComponentLayout::BGR,      8,                  8,                  8,                  0,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::B8G8R8_UINT,                 "VK_FORMAT_B8G8R8_UINT",                Anvil::ComponentLayout::BGR,      8,                  8,                  8,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::B8G8R8_SINT,                 "VK_FORMAT_B8G8R8_SINT",                Anvil::ComponentLayout::BGR,      8,                  8,                  8,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::B8G8R8_SRGB,                 "VK_FORMAT_B8G8R8_SRGB",                Anvil::ComponentLayout::BGR,      8,                  8,                  8,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::R8G8B8A8_UNORM,              "VK_FORMAT_R8G8B8A8_UNORM",             Anvil::ComponentLayout::RGBA,     8,                  8,                  8,                  8,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::R8G8B8A8_SNORM,              "VK_FORMAT_R8G8B8A8_SNORM",             Anvil::ComponentLayout::RGBA,     8,                  8,                  8,                  8,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::R8G8B8A8_USCALED,            "VK_FORMAT_R8G8B8A8_USCALED",           Anvil::ComponentLayout::RGBA,     8,                  8,                  8,                  8,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::R8G8B8A8_SSCALED,            "VK_FORMAT_R8G8B8A8_SSCALED",           Anvil::ComponentLayout::RGBA,     8,                  8,                  8,                  8,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::R8G8B8A8_UINT,               "VK_FORMAT_R8G8B8A8_UINT",              Anvil::ComponentLayout::RGBA,     8,                  8,                  8,                  8,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R8G8B8A8_SINT,               "VK_FORMAT_R8G8B8A8_SINT",              Anvil::ComponentLayout::RGBA,     8,                  8,                  8,                  8,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R8G8B8A8_SRGB,               "VK_FORMAT_R8G8B8A8_SRGB",              Anvil::ComponentLayout::RGBA,     8,                  8,                  8,                  8,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::B8G8R8A8_UNORM,              "VK_FORMAT_B8G8R8A8_UNORM",             Anvil::ComponentLayout::BGRA,     8,                  8,                  8,                  8,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::B8G8R8A8_SNORM,              "VK_FORMAT_B8G8R8A8_SNORM",             Anvil::ComponentLayout::BGRA,     8,                  8,                  8,                  8,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::B8G8R8A8_USCALED,            "VK_FORMAT_B8G8R8A8_USCALED",           Anvil::ComponentLayout::BGRA,     8,                  8,                  8,                  8,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::B8G8R8A8_SSCALED,            "VK_FORMAT_B8G8R8A8_SSCALED",           Anvil::ComponentLayout::BGRA,     8,                  8,                  8,                  8,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::B8G8R8A8_UINT,               "VK_FORMAT_B8G8R8A8_UINT",              Anvil::ComponentLayout::BGRA,     8,                  8,                  8,                  8,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::B8G8R8A8_SINT,               "VK_FORMAT_B8G8R8A8_SINT",              Anvil::ComponentLayout::BGRA,     8,                  8,                  8,                  8,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::B8G8R8A8_SRGB,               "VK_FORMAT_B8G8R8A8_SRGB",              Anvil::ComponentLayout::BGRA,     8,                  8,                  8,                  8,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::A8B8G8R8_UNORM_PACK32,       "VK_FORMAT_A8B8G8R8_UNORM_PACK32",      Anvil::ComponentLayout::ABGR,     8,                  8,                  8,                  8,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::A8B8G8R8_SNORM_PACK32,       "VK_FORMAT_A8B8G8R8_SNORM_PACK32",      Anvil::ComponentLayout::ABGR,     8,                  8,                  8,                  8,                  Anvil::FormatType::SNORM,       true},
    {Anvil::Format::A8B8G8R8_USCALED_PACK32,     "VK_FORMAT_A8B8G8R8_USCALED_PACK32",    Anvil::ComponentLayout::ABGR,     8,                  8,                  8,                  8,                  Anvil::FormatType::USCALED,     true},
    {Anvil::Format::A8B8G8R8_SSCALED_PACK32,     "VK_FORMAT_A8B8G8R8_SSCALED_PACK32",    Anvil::ComponentLayout::ABGR,     8,                  8,                  8,                  8,                  Anvil::FormatType::SSCALED,     true},
    {Anvil::Format::A8B8G8R8_UINT_PACK32,        "VK_FORMAT_A8B8G8R8_UINT_PACK32",       Anvil::ComponentLayout::ABGR,     8,                  8,                  8,                  8,                  Anvil::FormatType::UINT,        true},
    {Anvil::Format::A8B8G8R8_SINT_PACK32,        "VK_FORMAT_A8B8G8R8_SINT_PACK32",       Anvil::ComponentLayout::ABGR,     8,                  8,                  8,                  8,                  Anvil::FormatType::SINT,        true},
    {Anvil::Format::A8B8G8R8_SRGB_PACK32,        "VK_FORMAT_A8B8G8R8_SRGB_PACK32",       Anvil::ComponentLayout::ABGR,     8,                  8,                  8,                  8,                  Anvil::FormatType::SRGB,        true},
    {Anvil::Format::A2R10G10B10_UNORM_PACK32,    "VK_FORMAT_A2R10G10B10_UNORM_PACK32",   Anvil::ComponentLayout::ARGB,     2,                  10,                 10,                 10,                 Anvil::FormatType::UNORM,       true},
    {Anvil::Format::A2R10G10B10_SNORM_PACK32,    "VK_FORMAT_A2R10G10B10_SNORM_PACK32",   Anvil::ComponentLayout::ARGB,     2,                  10,                 10,                 10,                 Anvil::FormatType::SNORM,       true},
    {Anvil::Format::A2R10G10B10_USCALED_PACK32,  "VK_FORMAT_A2R10G10B10_USCALED_PACK32", Anvil::ComponentLayout::ARGB,     2,                  10,                 10,                 10,                 Anvil::FormatType::USCALED,     true},
    {Anvil::Format::A2R10G10B10_SSCALED_PACK32,  "VK_FORMAT_A2R10G10B10_SSCALED_PACK32", Anvil::ComponentLayout::ARGB,     2,                  10,                 10,                 10,                 Anvil::FormatType::SSCALED,     true},
    {Anvil::Format::A2R10G10B10_UINT_PACK32,     "VK_FORMAT_A2R10G10B10_UINT_PACK32",    Anvil::ComponentLayout::ARGB,     2,                  10,                 10,                 10,                 Anvil::FormatType::UINT,        true},
    {Anvil::Format::A2R10G10B10_SINT_PACK32,     "VK_FORMAT_A2R10G10B10_SINT_PACK32",    Anvil::ComponentLayout::ARGB,     2,                  10,                 10,                 10,                 Anvil::FormatType::SINT,        true},
    {Anvil::Format::A2B10G10R10_UNORM_PACK32,    "VK_FORMAT_A2B10G10R10_UNORM_PACK32",   Anvil::ComponentLayout::ABGR,     2,                  10,                 10,                 10,                 Anvil::FormatType::UNORM,       true},
    {Anvil::Format::A2B10G10R10_SNORM_PACK32,    "VK_FORMAT_A2B10G10R10_SNORM_PACK32",   Anvil::ComponentLayout::ABGR,     2,                  10,                 10,                 10,                 Anvil::FormatType::SNORM,       true},
    {Anvil::Format::A2B10G10R10_USCALED_PACK32,  "VK_FORMAT_A2B10G10R10_USCALED_PACK32", Anvil::ComponentLayout::ABGR,     2,                  10,                 10,                 10,                 Anvil::FormatType::USCALED,     true},
    {Anvil::Format::A2B10G10R10_SSCALED_PACK32,  "VK_FORMAT_A2B10G10R10_SSCALED_PACK32", Anvil::ComponentLayout::ABGR,     2,                  10,                 10,                 10,                 Anvil::FormatType::SSCALED,     true},
    {Anvil::Format::A2B10G10R10_UINT_PACK32,     "VK_FORMAT_A2B10G10R10_UINT_PACK32",    Anvil::ComponentLayout::ABGR,     2,                  10,                 10,                 10,                 Anvil::FormatType::UINT,        true},
    {Anvil::Format::A2B10G10R10_SINT_PACK32,     "VK_FORMAT_A2B10G10R10_SINT_PACK32",    Anvil::ComponentLayout::ABGR,     2,                  10,                 10,                 10,                 Anvil::FormatType::SINT,        true},
    {Anvil::Format::R16_UNORM,                   "VK_FORMAT_R16_UNORM",                  Anvil::ComponentLayout::R,        16,                 0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::R16_SNORM,                   "VK_FORMAT_R16_SNORM",                  Anvil::ComponentLayout::R,        16,                 0,                  0,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::R16_USCALED,                 "VK_FORMAT_R16_USCALED",                Anvil::ComponentLayout::R,        16,                 0,                  0,                  0,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::R16_SSCALED,                 "VK_FORMAT_R16_SSCALED",                Anvil::ComponentLayout::R,        16,                 0,                  0,                  0,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::R16_UINT,                    "VK_FORMAT_R16_UINT",                   Anvil::ComponentLayout::R,        16,                 0,                  0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R16_SINT,                    "VK_FORMAT_R16_SINT",                   Anvil::ComponentLayout::R,        16,                 0,                  0,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R16_SFLOAT,                  "VK_FORMAT_R16_SFLOAT",                 Anvil::ComponentLayout::R,        16,                 0,                  0,                  0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R16G16_UNORM,                "VK_FORMAT_R16G16_UNORM",               Anvil::ComponentLayout::RG,       16,                 16,                 0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::R16G16_SNORM,                "VK_FORMAT_R16G16_SNORM",               Anvil::ComponentLayout::RG,       16,                 16,                 0,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::R16G16_USCALED,              "VK_FORMAT_R16G16_USCALED",             Anvil::ComponentLayout::RG,       16,                 16,                 0,                  0,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::R16G16_SSCALED,              "VK_FORMAT_R16G16_SSCALED",             Anvil::ComponentLayout::RG,       16,                 16,                 0,                  0,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::R16G16_UINT,                 "VK_FORMAT_R16G16_UINT",                Anvil::ComponentLayout::RG,       16,                 16,                 0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R16G16_SINT,                 "VK_FORMAT_R16G16_SINT",                Anvil::ComponentLayout::RG,       16,                 16,                 0,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R16G16_SFLOAT,               "VK_FORMAT_R16G16_SFLOAT",              Anvil::ComponentLayout::RG,       16,                 16,                 0,                  0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R16G16B16_UNORM,             "VK_FORMAT_R16G16B16_UNORM",            Anvil::ComponentLayout::RGB,      16,                 16,                 16,                 0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::R16G16B16_SNORM,             "VK_FORMAT_R16G16B16_SNORM",            Anvil::ComponentLayout::RGB,      16,                 16,                 16,                 0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::R16G16B16_USCALED,           "VK_FORMAT_R16G16B16_USCALED",          Anvil::ComponentLayout::RGB,      16,                 16,                 16,                 0,                  Anvil::FormatType::USCALED,     false},
    {Anvil::Format::R16G16B16_SSCALED,           "VK_FORMAT_R16G16B16_SSCALED",          Anvil::ComponentLayout::RGB,      16,                 16,                 16,                 0,                  Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::R16G16B16_UINT,              "VK_FORMAT_R16G16B16_UINT",             Anvil::ComponentLayout::RGB,      16,                 16,                 16,                 0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R16G16B16_SINT,              "VK_FORMAT_R16G16B16_SINT",             Anvil::ComponentLayout::RGB,      16,                 16,                 16,                 0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R16G16B16_SFLOAT,            "VK_FORMAT_R16G16B16_SFLOAT",           Anvil::ComponentLayout::RGB,      16,                 16,                 16,                 0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R16G16B16A16_UNORM,          "VK_FORMAT_R16G16B16A16_UNORM",         Anvil::ComponentLayout::RGBA,     16,                 16,                 16,                 16,                 Anvil::FormatType::UNORM,       false},
    {Anvil::Format::R16G16B16A16_SNORM,          "VK_FORMAT_R16G16B16A16_SNORM",         Anvil::ComponentLayout::RGBA,     16,                 16,                 16,                 16,                 Anvil::FormatType::SNORM,       false},
    {Anvil::Format::R16G16B16A16_USCALED,        "VK_FORMAT_R16G16B16A16_USCALED",       Anvil::ComponentLayout::RGBA,     16,                 16,                 16,                 16,                 Anvil::FormatType::USCALED,     false},
    {Anvil::Format::R16G16B16A16_SSCALED,        "VK_FORMAT_R16G16B16A16_SSCALED",       Anvil::ComponentLayout::RGBA,     16,                 16,                 16,                 16,                 Anvil::FormatType::SSCALED,     false},
    {Anvil::Format::R16G16B16A16_UINT,           "VK_FORMAT_R16G16B16A16_UINT",          Anvil::ComponentLayout::RGBA,     16,                 16,                 16,                 16,                 Anvil::FormatType::UINT,        false},
    {Anvil::Format::R16G16B16A16_SINT,           "VK_FORMAT_R16G16B16A16_SINT",          Anvil::ComponentLayout::RGBA,     16,                 16,                 16,                 16,                 Anvil::FormatType::SINT,        false},
    {Anvil::Format::R16G16B16A16_SFLOAT,         "VK_FORMAT_R16G16B16A16_SFLOAT",        Anvil::ComponentLayout::RGBA,     16,                 16,                 16,                 16,                 Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R32_UINT,                    "VK_FORMAT_R32_UINT",                   Anvil::ComponentLayout::R,        32,                 0,                  0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R32_SINT,                    "VK_FORMAT_R32_SINT",                   Anvil::ComponentLayout::R,        32,                 0,                  0,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R32_SFLOAT,                  "VK_FORMAT_R32_SFLOAT",                 Anvil::ComponentLayout::R,        32,                 0,                  0,                  0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R32G32_UINT,                 "VK_FORMAT_R32G32_UINT",                Anvil::ComponentLayout::RG,       32,                 32,                 0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R32G32_SINT,                 "VK_FORMAT_R32G32_SINT",                Anvil::ComponentLayout::RG,       32,                 32,                 0,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R32G32_SFLOAT,               "VK_FORMAT_R32G32_SFLOAT",              Anvil::ComponentLayout::RG,       32,                 32,                 0,                  0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R32G32B32_UINT,              "VK_FORMAT_R32G32B32_UINT",             Anvil::ComponentLayout::RGB,      32,                 32,                 32,                 0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R32G32B32_SINT,              "VK_FORMAT_R32G32B32_SINT",             Anvil::ComponentLayout::RGB,      32,                 32,                 32,                 0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R32G32B32_SFLOAT,            "VK_FORMAT_R32G32B32_SFLOAT",           Anvil::ComponentLayout::RGB,      32,                 32,                 32,                 0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R32G32B32A32_UINT,           "VK_FORMAT_R32G32B32A32_UINT",          Anvil::ComponentLayout::RGBA,     32,                 32,                 32,                 32,                 Anvil::FormatType::UINT,        false},
    {Anvil::Format::R32G32B32A32_SINT,           "VK_FORMAT_R32G32B32A32_SINT",          Anvil::ComponentLayout::RGBA,     32,                 32,                 32,                 32,                 Anvil::FormatType::SINT,        false},
    {Anvil::Format::R32G32B32A32_SFLOAT,         "VK_FORMAT_R32G32B32A32_SFLOAT",        Anvil::ComponentLayout::RGBA,     32,                 32,                 32,                 32,                 Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R64_UINT,                    "VK_FORMAT_R64_UINT",                   Anvil::ComponentLayout::R,        64,                 0,                  0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R64_SINT,                    "VK_FORMAT_R64_SINT",                   Anvil::ComponentLayout::R,        64,                 0,                  0,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R64_SFLOAT,                  "VK_FORMAT_R64_SFLOAT",                 Anvil::ComponentLayout::R,        64,                 0,                  0,                  0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R64G64_UINT,                 "VK_FORMAT_R64G64_UINT",                Anvil::ComponentLayout::RG,       64,                 64,                 0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R64G64_SINT,                 "VK_FORMAT_R64G64_SINT",                Anvil::ComponentLayout::RG,       64,                 64,                 0,                  0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R64G64_SFLOAT,               "VK_FORMAT_R64G64_SFLOAT",              Anvil::ComponentLayout::RG,       64,                 64,                 0,                  0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R64G64B64_UINT,              "VK_FORMAT_R64G64B64_UINT",             Anvil::ComponentLayout::RGB,      64,                 64,                 64,                 0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::R64G64B64_SINT,              "VK_FORMAT_R64G64B64_SINT",             Anvil::ComponentLayout::RGB,      64,                 64,                 64,                 0,                  Anvil::FormatType::SINT,        false},
    {Anvil::Format::R64G64B64_SFLOAT,            "VK_FORMAT_R64G64B64_SFLOAT",           Anvil::ComponentLayout::RGB,      64,                 64,                 64,                 0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::R64G64B64A64_UINT,           "VK_FORMAT_R64G64B64A64_UINT",          Anvil::ComponentLayout::RGBA,     64,                 64,                 64,                 64,                 Anvil::FormatType::UINT,        false},
    {Anvil::Format::R64G64B64A64_SINT,           "VK_FORMAT_R64G64B64A64_SINT",          Anvil::ComponentLayout::RGBA,     64,                 64,                 64,                 64,                 Anvil::FormatType::SINT,        false},
    {Anvil::Format::R64G64B64A64_SFLOAT,         "VK_FORMAT_R64G64B64A64_SFLOAT",        Anvil::ComponentLayout::RGBA,     64,                 64,                 64,                 64,                 Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::B10G11R11_UFLOAT_PACK32,     "VK_FORMAT_B10G11R11_UFLOAT_PACK32",    Anvil::ComponentLayout::BGR,      10,                 11,                 11,                 0,                  Anvil::FormatType::UFLOAT,      true},
    {Anvil::Format::E5B9G9R9_UFLOAT_PACK32,      "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32",     Anvil::ComponentLayout::EBGR,     5,                  9,                  9,                  9,                  Anvil::FormatType::UFLOAT,      true},
    {Anvil::Format::D16_UNORM,                   "VK_FORMAT_D16_UNORM",                  Anvil::ComponentLayout::D,        16,                 0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::X8_D24_UNORM_PACK32,         "VK_FORMAT_X8_D24_UNORM_PACK32",        Anvil::ComponentLayout::XD,       8,                  24,                 0,                  0,                  Anvil::FormatType::UNORM,       true},
    {Anvil::Format::D32_SFLOAT,                  "VK_FORMAT_D32_SFLOAT",                 Anvil::ComponentLayout::D,        32,                 0,                  0,                  0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::S8_UINT,                     "VK_FORMAT_S8_UINT",                    Anvil::ComponentLayout::S,        8,                  0,                  0,                  0,                  Anvil::FormatType::UINT,        false},
    {Anvil::Format::D16_UNORM_S8_UINT,           "VK_FORMAT_D16_UNORM_S8_UINT",          Anvil::ComponentLayout::DS,       16,                 8,                  0,                  0,                  Anvil::FormatType::UNORM_UINT,  false},
    {Anvil::Format::D24_UNORM_S8_UINT,           "VK_FORMAT_D24_UNORM_S8_UINT",          Anvil::ComponentLayout::DS,       24,                 8,                  0,                  0,                  Anvil::FormatType::UNORM_UINT,  false},
    {Anvil::Format::D32_SFLOAT_S8_UINT,          "VK_FORMAT_D32_SFLOAT_S8_UINT",         Anvil::ComponentLayout::DS,       32,                 8,                  0,                  0,                  Anvil::FormatType::SFLOAT_UINT, false},
    {Anvil::Format::BC1_RGB_UNORM_BLOCK,         "VK_FORMAT_BC1_RGB_UNORM_BLOCK",        Anvil::ComponentLayout::RGB,      0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::BC1_RGB_SRGB_BLOCK,          "VK_FORMAT_BC1_RGB_SRGB_BLOCK",         Anvil::ComponentLayout::RGB,      0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::BC1_RGBA_UNORM_BLOCK,        "VK_FORMAT_BC1_RGBA_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::BC1_RGBA_SRGB_BLOCK,         "VK_FORMAT_BC1_RGBA_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::BC2_UNORM_BLOCK,             "VK_FORMAT_BC2_UNORM_BLOCK",            Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::BC2_SRGB_BLOCK,              "VK_FORMAT_BC2_SRGB_BLOCK",             Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::BC3_UNORM_BLOCK,             "VK_FORMAT_BC3_UNORM_BLOCK",            Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::BC3_SRGB_BLOCK,              "VK_FORMAT_BC3_SRGB_BLOCK",             Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::BC4_UNORM_BLOCK,             "VK_FORMAT_BC4_UNORM_BLOCK",            Anvil::ComponentLayout::R,        0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::BC4_SNORM_BLOCK,             "VK_FORMAT_BC4_SNORM_BLOCK",            Anvil::ComponentLayout::R,        0,                  0,                  0,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::BC5_UNORM_BLOCK,             "VK_FORMAT_BC5_UNORM_BLOCK",            Anvil::ComponentLayout::RG,       0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::BC5_SNORM_BLOCK,             "VK_FORMAT_BC5_SNORM_BLOCK",            Anvil::ComponentLayout::RG,       0,                  0,                  0,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::BC6H_UFLOAT_BLOCK,           "VK_FORMAT_BC6H_UFLOAT_BLOCK",          Anvil::ComponentLayout::RGB,      0,                  0,                  0,                  0,                  Anvil::FormatType::UFLOAT,      false},
    {Anvil::Format::BC6H_SFLOAT_BLOCK,           "VK_FORMAT_BC6H_SFLOAT_BLOCK",          Anvil::ComponentLayout::RGB,      0,                  0,                  0,                  0,                  Anvil::FormatType::SFLOAT,      false},
    {Anvil::Format::BC7_UNORM_BLOCK,             "VK_FORMAT_BC7_UNORM_BLOCK",            Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::BC7_SRGB_BLOCK,              "VK_FORMAT_BC7_SRGB_BLOCK",             Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ETC2_R8G8B8_UNORM_BLOCK,     "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK",    Anvil::ComponentLayout::RGB,      0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ETC2_R8G8B8_SRGB_BLOCK,      "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK",     Anvil::ComponentLayout::RGB,      0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ETC2_R8G8B8A1_UNORM_BLOCK,   "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK",  Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ETC2_R8G8B8A1_SRGB_BLOCK,    "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK",   Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ETC2_R8G8B8A8_UNORM_BLOCK,   "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK",  Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ETC2_R8G8B8A8_SRGB_BLOCK,    "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK",   Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::EAC_R11_UNORM_BLOCK,         "VK_FORMAT_EAC_R11_UNORM_BLOCK",        Anvil::ComponentLayout::R,        0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::EAC_R11_SNORM_BLOCK,         "VK_FORMAT_EAC_R11_SNORM_BLOCK",        Anvil::ComponentLayout::R,        0,                  0,                  0,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::EAC_R11G11_UNORM_BLOCK,      "VK_FORMAT_EAC_R11G11_UNORM_BLOCK",     Anvil::ComponentLayout::RG,       0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::EAC_R11G11_SNORM_BLOCK,      "VK_FORMAT_EAC_R11G11_SNORM_BLOCK",     Anvil::ComponentLayout::RG,       0,                  0,                  0,                  0,                  Anvil::FormatType::SNORM,       false},
    {Anvil::Format::ASTC_4x4_UNORM_BLOCK,        "VK_FORMAT_ASTC_4x4_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_4x4_SRGB_BLOCK,         "VK_FORMAT_ASTC_4x4_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_5x4_UNORM_BLOCK,        "VK_FORMAT_ASTC_5x4_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_5x4_SRGB_BLOCK,         "VK_FORMAT_ASTC_5x4_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_5x5_UNORM_BLOCK,        "VK_FORMAT_ASTC_5x5_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_5x5_SRGB_BLOCK,         "VK_FORMAT_ASTC_5x5_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_6x5_UNORM_BLOCK,        "VK_FORMAT_ASTC_6x5_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_6x5_SRGB_BLOCK,         "VK_FORMAT_ASTC_6x5_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_6x6_UNORM_BLOCK,        "VK_FORMAT_ASTC_6x6_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_6x6_SRGB_BLOCK,         "VK_FORMAT_ASTC_6x6_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_8x5_UNORM_BLOCK,        "VK_FORMAT_ASTC_8x5_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_8x5_SRGB_BLOCK,         "VK_FORMAT_ASTC_8x5_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_8x6_UNORM_BLOCK,        "VK_FORMAT_ASTC_8x6_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_8x6_SRGB_BLOCK,         "VK_FORMAT_ASTC_8x6_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_8x8_UNORM_BLOCK,        "VK_FORMAT_ASTC_8x8_UNORM_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_8x8_SRGB_BLOCK,         "VK_FORMAT_ASTC_8x8_SRGB_BLOCK",        Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_10x5_UNORM_BLOCK,       "VK_FORMAT_ASTC_10x5_UNORM_BLOCK",      Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_10x5_SRGB_BLOCK,        "VK_FORMAT_ASTC_10x5_SRGB_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_10x6_UNORM_BLOCK,       "VK_FORMAT_ASTC_10x6_UNORM_BLOCK",      Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_10x6_SRGB_BLOCK,        "VK_FORMAT_ASTC_10x6_SRGB_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_10x8_UNORM_BLOCK,       "VK_FORMAT_ASTC_10x8_UNORM_BLOCK",      Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_10x8_SRGB_BLOCK,        "VK_FORMAT_ASTC_10x8_SRGB_BLOCK",       Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_10x10_UNORM_BLOCK,      "VK_FORMAT_ASTC_10x10_UNORM_BLOCK",     Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_10x10_SRGB_BLOCK,       "VK_FORMAT_ASTC_10x10_SRGB_BLOCK",      Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_12x10_UNORM_BLOCK,      "VK_FORMAT_ASTC_12x10_UNORM_BLOCK",     Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_12x10_SRGB_BLOCK,       "VK_FORMAT_ASTC_12x10_SRGB_BLOCK",      Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false},
    {Anvil::Format::ASTC_12x12_UNORM_BLOCK,      "VK_FORMAT_ASTC_12x12_UNORM_BLOCK",     Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::UNORM,       false},
    {Anvil::Format::ASTC_12x12_SRGB_BLOCK,       "VK_FORMAT_ASTC_12x12_SRGB_BLOCK",      Anvil::ComponentLayout::RGBA,     0,                  0,                  0,                  0,                  Anvil::FormatType::SRGB,        false}
};

struct SubresourceLayoutInfo
{
    Anvil::Format          compatible_singleplanar_format;
    Anvil::ComponentLayout component_layout;
    uint8_t                component_bits_used  [4];
    uint8_t                component_bits_unused[4];

    /*  Default Constructor  */
    SubresourceLayoutInfo()
    {
        compatible_singleplanar_format = Anvil::Format::UNKNOWN;
        component_layout               = Anvil::ComponentLayout::UNKNOWN;
        component_bits_used  [0]       = component_bits_used  [1] = component_bits_used  [2] = component_bits_used  [3] = 0;
        component_bits_unused[0]       = component_bits_unused[1] = component_bits_unused[2] = component_bits_unused[3] = 0;
    }

    SubresourceLayoutInfo(const Anvil::Format&   in_compatible_singleplanar_format,
                          Anvil::ComponentLayout in_component_layout,
                          uint8_t                in_component0_bits,
                          uint8_t                in_component1_bits,
                          uint8_t                in_component2_bits,
                          uint8_t                in_component3_bits)
    {
        compatible_singleplanar_format = in_compatible_singleplanar_format;
        component_layout               = in_component_layout;
        component_bits_used[0]         = in_component0_bits;
        component_bits_used[1]         = in_component1_bits;
        component_bits_used[2]         = in_component2_bits;
        component_bits_used[3]         = in_component3_bits;

        component_bits_unused[0] = component_bits_unused[1] = component_bits_unused[2] = component_bits_unused[3] = 0;
    }

    /* Constructor for packed format */
    SubresourceLayoutInfo(const Anvil::Format&   in_compatible_singleplanar_format,
                          Anvil::ComponentLayout in_component_layout,
                          uint8_t                in_component0_bits_used,
                          uint8_t                in_component1_bits_used,
                          uint8_t                in_component2_bits_used,
                          uint8_t                in_component3_bits_used,
                          uint8_t                in_n_bits_per_component)
    {
        compatible_singleplanar_format = in_compatible_singleplanar_format;
        component_layout               = in_component_layout;
        component_bits_used[0]         = in_component0_bits_used;
        component_bits_used[1]         = in_component1_bits_used;
        component_bits_used[2]         = in_component2_bits_used;
        component_bits_used[3]         = in_component3_bits_used;

        component_bits_unused[0] = component_bits_unused[1] = component_bits_unused[2] = component_bits_unused[3] = 0;

        if (in_component0_bits_used != 0)
        {
            anvil_assert(in_component0_bits_used <= in_n_bits_per_component);

            component_bits_unused[0] = in_n_bits_per_component - in_component0_bits_used;
        }

        if (in_component1_bits_used != 0)
        {
            anvil_assert(in_component1_bits_used <= in_n_bits_per_component);

            component_bits_unused[1] = in_n_bits_per_component - in_component1_bits_used;
        }

        if (in_component2_bits_used != 0)
        {
            anvil_assert(in_component2_bits_used <= in_n_bits_per_component);

            component_bits_unused[2] = in_n_bits_per_component - in_component2_bits_used;
        }

        if (in_component3_bits_used != 0)
        {
            anvil_assert(in_component3_bits_used <= in_n_bits_per_component);

            component_bits_unused[3] = in_n_bits_per_component - in_component3_bits_used;
        }
    }

};

struct YUVFormatInfo
{
    const char*                        name;
    uint8_t                            num_planes;
    std::vector<SubresourceLayoutInfo> subresources;
    Anvil::FormatType                  format_type;
    bool                               is_multiplanar;
    bool                               is_packed;

    YUVFormatInfo()
        :name          (nullptr),
         num_planes    (0),
         format_type   (Anvil::FormatType::UNKNOWN),
         is_multiplanar(false),
         is_packed     (false)
    {
        /* Stub */
    }

    YUVFormatInfo(const char*           in_name,
                  uint8_t               in_num_planes,
                  SubresourceLayoutInfo in_subresource0,
                  SubresourceLayoutInfo in_subresource1,
                  SubresourceLayoutInfo in_subresource2,
                  Anvil::FormatType     in_format_type,
                  bool                  in_multiplanar,
                  bool                  in_packed)
        : name          (in_name),
          num_planes    (in_num_planes),
          format_type   (in_format_type),
          is_multiplanar(in_multiplanar),
          is_packed     (in_packed)
    {
        subresources.push_back(in_subresource0);
        subresources.push_back(in_subresource1);
        subresources.push_back(in_subresource2);
    }
};

/* TODO: Component layouts are wrong for YUV formats? */
static const std::unordered_map<Anvil::Format, YUVFormatInfo, Anvil::EnumClassHasher<Anvil::Format>> g_yuv_formats =
{
    /* format                                                    | name                                                   | num_planes | subresources[0]                                                                                          | subresources[1]                                                                                 | subresources[2]                                                                  | format_type              | is_multiplanar? | is_packed? */
    {Anvil::Format::G8B8G8R8_422_UNORM,                          {"VK_FORMAT_G8B8G8R8_422_UNORM",                         1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::GBGR,     8,      8,      8,      8},          {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            false} },
    {Anvil::Format::B8G8R8G8_422_UNORM,                          {"VK_FORMAT_B8G8R8G8_422_UNORM",                         1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::BGRG,     8,      8,      8,      8},          {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            false} },
    {Anvil::Format::G8_B8_R8_3PLANE_420_UNORM,                   {"VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM",                  3,             {Anvil::Format::R8_UNORM,           Anvil::ComponentLayout::R,        8,      0,      0,      0},          {Anvil::Format::R8_UNORM,                 Anvil::ComponentLayout::R,    8,   0,   0,   0},        {Anvil::Format::R8_UNORM, Anvil::ComponentLayout::R,  8,  0, 0, 0},                Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::G8_B8R8_2PLANE_420_UNORM,                    {"VK_FORMAT_G8_B8R8_2PLANE_420_UNORM",                   2,             {Anvil::Format::R8_UNORM,           Anvil::ComponentLayout::R,        8,      0,      0,      0},          {Anvil::Format::R8G8_UNORM,               Anvil::ComponentLayout::BR,   8,   8,   0,   0},        {},                                                                                Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::G8_B8_R8_3PLANE_422_UNORM,                   {"VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM",                  3,             {Anvil::Format::R8_UNORM,           Anvil::ComponentLayout::G,        8,      0,      0,      0},          {Anvil::Format::R8_UNORM,                 Anvil::ComponentLayout::B,    8,   0,   0,   0},        {Anvil::Format::R8_UNORM, Anvil::ComponentLayout::R,  8,  0, 0, 0},                Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::G8_B8R8_2PLANE_422_UNORM,                    {"VK_FORMAT_G8_B8R8_2PLANE_422_UNORM",                   2,             {Anvil::Format::R8_UNORM,           Anvil::ComponentLayout::G,        8,      0,      0,      0},          {Anvil::Format::R8G8_UNORM,               Anvil::ComponentLayout::BR,   8,   8,   0,   0},        {},                                                                                Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::G8_B8_R8_3PLANE_444_UNORM,                   {"VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM",                  3,             {Anvil::Format::R8_UNORM,           Anvil::ComponentLayout::G,        8,      0,      0,      0},          {Anvil::Format::R8_UNORM,                 Anvil::ComponentLayout::B,    8,   0,   0,   0},        {Anvil::Format::R8_UNORM, Anvil::ComponentLayout::R,  8,  0, 0, 0},                Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::R10X6_UNORM_PACK16,                          {"VK_FORMAT_R10X6_UNORM_PACK16",                         1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::RX,       10,     0,      0,      0,    16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::R10X6G10X6_UNORM_2PACK16,                    {"VK_FORMAT_R10X6G10X6_UNORM_2PACK16",                   1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::RXGX,     10,     10,     0,      0,    16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16,          {"VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16",         1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::RXGXBXAX, 10,     10,     10,     10,   16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,      {"VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16",     1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::GXBXGXRX, 10,     10,     10,     10,   16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,      {"VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16",     1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::BXGXRXGX, 10,     10,     10,     10,   16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,  {"VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16", 3,             {Anvil::Format::R10X6_UNORM_PACK16, Anvil::ComponentLayout::GX,       10,     0,      0,      0,    16},   {Anvil::Format::R10X6_UNORM_PACK16,       Anvil::ComponentLayout::BX,   10,  0,   0,   0,  16},   {Anvil::Format::R10X6_UNORM_PACK16, Anvil::ComponentLayout::RX, 10, 0, 0, 0, 16},  Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,   {"VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16",  2,             {Anvil::Format::R10X6_UNORM_PACK16, Anvil::ComponentLayout::GX,       10,     0,      0,      0,    16},   {Anvil::Format::R10X6G10X6_UNORM_2PACK16, Anvil::ComponentLayout::BXRX, 10,  10,  0,   0,  16},   {},                                                                                Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,  {"VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16", 3,             {Anvil::Format::R10X6_UNORM_PACK16, Anvil::ComponentLayout::GX,       10,     0,      0,      0,    16},   {Anvil::Format::R10X6_UNORM_PACK16,       Anvil::ComponentLayout::BX,   10,  0,   0,   0,  16},   {Anvil::Format::R10X6_UNORM_PACK16, Anvil::ComponentLayout::RX, 10, 0, 0, 0, 16},  Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,   {"VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16",  2,             {Anvil::Format::R10X6_UNORM_PACK16, Anvil::ComponentLayout::GX,       10,     0,      0,      0,    16},   {Anvil::Format::R10X6G10X6_UNORM_2PACK16, Anvil::ComponentLayout::BXRX, 10,  10,  0,   0,  16},   {},                                                                                Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,  {"VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16", 3,             {Anvil::Format::R10X6_UNORM_PACK16, Anvil::ComponentLayout::GX,       10,     0,      0,      0,    16},   {Anvil::Format::R10X6_UNORM_PACK16,       Anvil::ComponentLayout::BX,   10,  0,   0,   0,  16},   {Anvil::Format::R10X6_UNORM_PACK16, Anvil::ComponentLayout::RX, 10, 0, 0, 0, 16},  Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::R12X4_UNORM_PACK16,                          {"VK_FORMAT_R12X4_UNORM_PACK16",                         1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::RX,       12,     0,      0,      0,    16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::R12X4G12X4_UNORM_2PACK16,                    {"VK_FORMAT_R12X4G12X4_UNORM_2PACK16",                   1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::RXGX,     12,     12,     0,      0,    16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16,          {"VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16",         1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::RXGXBXAX, 12,     12,     12,     12,   16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,      {"VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16",     1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::GXBXGXRX, 12,     12,     12,     12,   16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,      {"VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16",     1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::BXGXRXGX, 12,     12,     12,     12,   16},   {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            true } },
    {Anvil::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,  {"VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16", 3,             {Anvil::Format::R12X4_UNORM_PACK16, Anvil::ComponentLayout::GX,       12,     0,      0,      0,    16},   {Anvil::Format::R12X4_UNORM_PACK16,       Anvil::ComponentLayout::BX,   12,  0,   0,   0,  16},   {Anvil::Format::R12X4_UNORM_PACK16, Anvil::ComponentLayout::RX, 12, 0, 0, 0, 16},  Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,   {"VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16",  2,             {Anvil::Format::R12X4_UNORM_PACK16, Anvil::ComponentLayout::GX,       12,     0,      0,      0,    16},   {Anvil::Format::R12X4G12X4_UNORM_2PACK16, Anvil::ComponentLayout::BXRX, 12,  12,  0,   0,  16},   {},                                                                                Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,  {"VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16", 3,             {Anvil::Format::R12X4_UNORM_PACK16, Anvil::ComponentLayout::GX,       12,     0,      0,      0,    16},   {Anvil::Format::R12X4_UNORM_PACK16,       Anvil::ComponentLayout::BX,   12,  0,   0,   0,  16},   {Anvil::Format::R12X4_UNORM_PACK16, Anvil::ComponentLayout::RX, 12, 0, 0, 0, 16},  Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,   {"VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16",  2,             {Anvil::Format::R12X4_UNORM_PACK16, Anvil::ComponentLayout::GX,       12,     0,      0,      0,    16},   {Anvil::Format::R12X4G12X4_UNORM_2PACK16, Anvil::ComponentLayout::BXRX, 12,  12,  0,   0,  16},   {},                                                                                Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,  {"VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16", 3,             {Anvil::Format::R12X4_UNORM_PACK16, Anvil::ComponentLayout::GX,       12,     0,      0,      0,    16},   {Anvil::Format::R12X4_UNORM_PACK16,       Anvil::ComponentLayout::BX,   12,  0,   0,   0,  16},   {Anvil::Format::R12X4_UNORM_PACK16, Anvil::ComponentLayout::RX, 12, 0, 0, 0, 16},  Anvil::FormatType::UNORM,  true,             true } },
    {Anvil::Format::G16B16G16R16_422_UNORM,                      {"VK_FORMAT_G16B16G16R16_422_UNORM",                     1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::GBGR,     16,     16,     16,     16},         {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            false} },
    {Anvil::Format::B16G16R16G16_422_UNORM,                      {"VK_FORMAT_B16G16R16G16_422_UNORM",                     1,             {Anvil::Format::UNKNOWN,            Anvil::ComponentLayout::BGRG,     16,     16,     16,     16},         {},                                                                                               {},                                                                                Anvil::FormatType::UNORM,  false,            false} },
    {Anvil::Format::G16_B16_R16_3PLANE_420_UNORM,                {"VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM",               3,             {Anvil::Format::R16_UNORM,          Anvil::ComponentLayout::G,        16,     0,      0,      0},          {Anvil::Format::R16_UNORM,                Anvil::ComponentLayout::B,    16,  0,   0,   0},        {Anvil::Format::R16_UNORM, Anvil::ComponentLayout::R,  16, 0, 0, 0},               Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::G16_B16R16_2PLANE_420_UNORM,                 {"VK_FORMAT_G16_B16R16_2PLANE_420_UNORM",                2,             {Anvil::Format::R16_UNORM,          Anvil::ComponentLayout::G,        16,     0,      0,      0},          {Anvil::Format::R16G16_UNORM,             Anvil::ComponentLayout::BR,   16,  16,  0,   0},        {},                                                                                Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::G16_B16_R16_3PLANE_422_UNORM,                {"VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM",               3,             {Anvil::Format::R16_UNORM,          Anvil::ComponentLayout::G,        16,     0,      0,      0},          {Anvil::Format::R16_UNORM,                Anvil::ComponentLayout::B,    16,  0,   0,   0},        {Anvil::Format::R16_UNORM, Anvil::ComponentLayout::R,  16, 0, 0, 0},               Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::G16_B16R16_2PLANE_422_UNORM,                 {"VK_FORMAT_G16_B16R16_2PLANE_422_UNORM",                2,             {Anvil::Format::R16_UNORM,          Anvil::ComponentLayout::G,        16,     0,      0,      0},          {Anvil::Format::R16G16_UNORM,             Anvil::ComponentLayout::BR,   16,  16,  0,   0},        {},                                                                                Anvil::FormatType::UNORM,  true,             false} },
    {Anvil::Format::G16_B16_R16_3PLANE_444_UNORM,                {"VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM",               3,             {Anvil::Format::R16_UNORM,          Anvil::ComponentLayout::G,        16,     0,      0,      0},          {Anvil::Format::R16_UNORM,                Anvil::ComponentLayout::B,    16,  0,   0,   0},        {Anvil::Format::R16_UNORM, Anvil::ComponentLayout::R,  16, 0, 0, 0},               Anvil::FormatType::UNORM,  true,             false} },
};

typedef struct
{
    uint32_t red_component_start_bit_index;
    uint32_t red_component_last_bit_index;
    uint32_t green_component_start_bit_index;
    uint32_t green_component_last_bit_index;
    uint32_t blue_component_start_bit_index;
    uint32_t blue_component_last_bit_index;
    uint32_t alpha_component_start_bit_index;
    uint32_t alpha_component_last_bit_index;
    uint32_t shared_component_start_bit_index;
    uint32_t shared_component_last_bit_index;
    uint32_t depth_component_start_bit_index;
    uint32_t depth_component_last_bit_index;
    uint32_t stencil_component_start_bit_index;
    uint32_t stencil_component_last_bit_index;
} NonYUVFormatBitLayoutInfo;

typedef struct
{
    uint32_t plane0_r0_start_bit_index;
    uint32_t plane0_r0_last_bit_index;
    uint32_t plane0_g0_start_bit_index;
    uint32_t plane0_g0_last_bit_index;
    uint32_t plane0_b0_start_bit_index;
    uint32_t plane0_b0_last_bit_index;
    uint32_t plane0_a0_start_bit_index;
    uint32_t plane0_a0_last_bit_index;

    uint32_t plane0_g1_start_bit_index;
    uint32_t plane0_g1_last_bit_index;

    uint32_t plane1_r0_start_bit_index;
    uint32_t plane1_r0_last_bit_index;
    uint32_t plane1_g0_start_bit_index;
    uint32_t plane1_g0_last_bit_index;
    uint32_t plane1_b0_start_bit_index;
    uint32_t plane1_b0_last_bit_index;

    uint32_t plane2_r0_start_bit_index;
    uint32_t plane2_r0_last_bit_index;
    uint32_t plane2_g0_start_bit_index;
    uint32_t plane2_g0_last_bit_index;
    uint32_t plane2_b0_start_bit_index;
    uint32_t plane2_b0_last_bit_index;
} YUVFormatBitLayoutInfo;

static const std::unordered_map<Anvil::Format, YUVFormatBitLayoutInfo, Anvil::EnumClassHasher<Anvil::Format> > g_yuv_format_bit_layout_info =
{
    /*                      Single-planar non-packed YUV formats ==> */

    /* format                                                  | p0r0 start | p0r0 end  | p0g0 start | p0g0 end  | p0b0 start | p0b0 end  | p0a0 start | p0a0 end  | p0g1 start | p0g1 end  | p1r0 start | p1r0 end  | p1g0 start | p1g0 end   | p1b0 start | p1b0 end  | p2r0 start | p2r0 end  | p2g0 start | p2g0 end  | p2b0 start | p2b0 end */
    {Anvil::Format::G8B8G8R8_422_UNORM,                         {24,          31,         0,           7,          8,           15,         UINT32_MAX,  UINT32_MAX, 16,          23,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::B8G8R8G8_422_UNORM,                         {16,          23,         8,           15,         0,           7,          UINT32_MAX,  UINT32_MAX, 24,          31,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G16B16G16R16_422_UNORM,                     {48,          63,         0,           15,         16,          31,         UINT32_MAX,  UINT32_MAX, 32,          47,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::B16G16R16G16_422_UNORM,                     {32,          47,         16,          31,         0,           15,         UINT32_MAX,  UINT32_MAX, 48,          63,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },

    /*                      Single-planar packed YUV formats ==> */
    {Anvil::Format::R10X6_UNORM_PACK16,                         {6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::R10X6G10X6_UNORM_2PACK16,                   {22,          31,         6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16,         {54,          63,         38,          47,         22,          31,         6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,     {48,          57,         0,           9,          16,          25,         UINT32_MAX,  UINT32_MAX, 32,          41,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,     {32,          41,         16,          25,         0,           9,          UINT32_MAX,  UINT32_MAX, 48,          57,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::R12X4_UNORM_PACK16,                         {4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::R12X4G12X4_UNORM_2PACK16,                   {20,          31,         4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16,         {52,          63,         36,          47,         20,          31,         4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,     {48,          59,         0,           11,         16,          27,         UINT32_MAX,  UINT32_MAX, 32,          43,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,     {32,          43,         16,          27,         0,           11,         UINT32_MAX,  UINT32_MAX, 48,          59,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },

    /*                      Multi-planar non-packed YUV formats ==> */

    {Anvil::Format::G8_B8_R8_3PLANE_420_UNORM,                  {UINT32_MAX,  UINT32_MAX, 0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  0,           7,          0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G8_B8R8_2PLANE_420_UNORM,                   {UINT32_MAX,  UINT32_MAX, 0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, 8,           15,         UINT32_MAX,  UINT32_MAX,  0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G8_B8_R8_3PLANE_422_UNORM,                  {UINT32_MAX,  UINT32_MAX, 0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  0,           7,          0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G8_B8R8_2PLANE_422_UNORM,                   {UINT32_MAX,  UINT32_MAX, 0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, 8,           15,         UINT32_MAX,  UINT32_MAX,  0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G8_B8_R8_3PLANE_444_UNORM,                  {UINT32_MAX,  UINT32_MAX, 0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  0,           7,          0,           7,          UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G16_B16_R16_3PLANE_420_UNORM,               {UINT32_MAX,  UINT32_MAX, 0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  0,           15,         0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G16_B16R16_2PLANE_420_UNORM,                {UINT32_MAX,  UINT32_MAX, 0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, 16,          31,         UINT32_MAX,  UINT32_MAX,  0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G16_B16_R16_3PLANE_422_UNORM,               {UINT32_MAX,  UINT32_MAX, 0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  0,           15,         0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G16_B16R16_2PLANE_422_UNORM,                {UINT32_MAX,  UINT32_MAX, 0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, 16,          31,         UINT32_MAX,  UINT32_MAX,  0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G16_B16_R16_3PLANE_444_UNORM,               {UINT32_MAX,  UINT32_MAX, 0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  0,           15,         0,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },

    /*                      Multi-planar packed YUV formats ==> */

    {Anvil::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, {UINT32_MAX,  UINT32_MAX, 6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  6,           15,         6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,  {UINT32_MAX,  UINT32_MAX, 6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, 6,           15,         UINT32_MAX,  UINT32_MAX,  22,          31,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, {UINT32_MAX,  UINT32_MAX, 6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  6,           15,         6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,  {UINT32_MAX,  UINT32_MAX, 6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, 6,           15,         UINT32_MAX,  UINT32_MAX,  22,          31,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, {UINT32_MAX,  UINT32_MAX, 6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  6,           15,         6,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, {UINT32_MAX,  UINT32_MAX, 4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  4,           15,         4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,  {UINT32_MAX,  UINT32_MAX, 4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, 4,           15,         UINT32_MAX,  UINT32_MAX,  20,          31,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, {UINT32_MAX,  UINT32_MAX, 4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  4,           15,         4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,  {UINT32_MAX,  UINT32_MAX, 4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, 4,           15,         UINT32_MAX,  UINT32_MAX,  20,          31,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
    {Anvil::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, {UINT32_MAX,  UINT32_MAX, 4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX,  4,           15,         4,           15,         UINT32_MAX,  UINT32_MAX, UINT32_MAX,  UINT32_MAX} },
};

static const std::unordered_map<Anvil::Format, NonYUVFormatBitLayoutInfo, Anvil::EnumClassHasher<Anvil::Format> > g_nonyuv_format_bit_layout_info =
{
    /* format                                   | red start | red end   | green start | green end | blue start | blue end   | alpha start | alpha end | shared_start | shared_end | depth start | depth end | stencil start | stencil end */
    {Anvil::Format::UNKNOWN,                     {UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R4G4_UNORM_PACK8,            {4,          7,          0,            3,          UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R4G4B4A4_UNORM_PACK16,       {12,         15,         8,            11,         4,           7,           0,            3,          UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B4G4R4A4_UNORM_PACK16,       {4,          7,          8,            11,         12,          15,          0,            3,          UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R5G6B5_UNORM_PACK16,         {11,         15,         5,            10,         0,           4,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B5G6R5_UNORM_PACK16,         {0,          4,          5,            10,         11,          15,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R5G5B5A1_UNORM_PACK16,       {11,         15,         6,            10,         1,           5,           0,            0,          UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B5G5R5A1_UNORM_PACK16,       {1,          5,          6,            10,         11,          15,          0,            0,          UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A1R5G5B5_UNORM_PACK16,       {10,         14,         5,            9,          0,           4,           15,           15,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8_UNORM,                    {0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8_SNORM,                    {0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8_USCALED,                  {0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8_SSCALED,                  {0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8_UINT,                     {0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8_SINT,                     {0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8_SRGB,                     {0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8_UNORM,                  {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8_SNORM,                  {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8_USCALED,                {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8_SSCALED,                {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8_UINT,                   {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8_SINT,                   {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8_SRGB,                   {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8_UNORM,                {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8_SNORM,                {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8_USCALED,              {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8_SSCALED,              {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8_UINT,                 {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8_SINT,                 {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8_SRGB,                 {0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8_UNORM,                {16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8_SNORM,                {16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8_USCALED,              {16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8_SSCALED,              {16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8_UINT,                 {16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8_SINT,                 {16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8_SRGB,                 {16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8A8_UNORM,              {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8A8_SNORM,              {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8A8_USCALED,            {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8A8_SSCALED,            {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8A8_UINT,               {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8A8_SINT,               {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R8G8B8A8_SRGB,               {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8A8_UNORM,              {16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8A8_SNORM,              {16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8A8_USCALED,            {16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8A8_SSCALED,            {16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8A8_UINT,               {16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8A8_SINT,               {16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B8G8R8A8_SRGB,               {16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A8B8G8R8_UNORM_PACK32,       {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A8B8G8R8_SNORM_PACK32,       {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A8B8G8R8_USCALED_PACK32,     {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A8B8G8R8_SSCALED_PACK32,     {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A8B8G8R8_UINT_PACK32,        {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A8B8G8R8_SINT_PACK32,        {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A8B8G8R8_SRGB_PACK32,        {0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2R10G10B10_UNORM_PACK32,    {20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2R10G10B10_SNORM_PACK32,    {20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2R10G10B10_USCALED_PACK32,  {20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2R10G10B10_SSCALED_PACK32,  {20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2R10G10B10_UINT_PACK32,     {20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2R10G10B10_SINT_PACK32,     {20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2B10G10R10_UNORM_PACK32,    {0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2B10G10R10_SNORM_PACK32,    {0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2B10G10R10_USCALED_PACK32,  {0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2B10G10R10_SSCALED_PACK32,  {0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2B10G10R10_UINT_PACK32,     {0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::A2B10G10R10_SINT_PACK32,     {0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16_UNORM,                   {0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16_SNORM,                   {0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16_USCALED,                 {0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16_SSCALED,                 {0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16_UINT,                    {0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16_SINT,                    {0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16_SFLOAT,                  {0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16_UNORM,                {0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16_SNORM,                {0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16_USCALED,              {0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16_SSCALED,              {0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16_UINT,                 {0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16_SINT,                 {0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16_SFLOAT,               {0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16_UNORM,             {0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16_SNORM,             {0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16_USCALED,           {0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16_SSCALED,           {0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16_UINT,              {0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16_SINT,              {0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16_SFLOAT,            {0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16A16_UNORM,          {0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16A16_SNORM,          {0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16A16_USCALED,        {0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16A16_SSCALED,        {0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16A16_UINT,           {0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16A16_SINT,           {0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R16G16B16A16_SFLOAT,         {0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32_UINT,                    {0,          31,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32_SINT,                    {0,          31,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32_SFLOAT,                  {0,          31,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32_UINT,                 {0,          31,         32,           63,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32_SINT,                 {0,          31,         32,           63,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32_SFLOAT,               {0,          31,         32,           63,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32B32_UINT,              {0,          31,         32,           63,         64,          95,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32B32_SINT,              {0,          31,         32,           63,         64,          95,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32B32_SFLOAT,            {0,          31,         32,           63,         64,          95,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32B32A32_UINT,           {0,          31,         32,           63,         64,          95,          96,           127,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32B32A32_SINT,           {0,          31,         32,           63,         64,          95,          96,           127,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R32G32B32A32_SFLOAT,         {0,          31,         32,           63,         64,          95,          96,           127,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64_UINT,                    {0,          63,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64_SINT,                    {0,          63,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64_SFLOAT,                  {0,          63,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64_UINT,                 {0,          63,         64,           127,        UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64_SINT,                 {0,          63,         64,           127,        UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64_SFLOAT,               {0,          63,         64,           127,        UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64B64_UINT,              {0,          63,         64,           127,        128,         191,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64B64_SINT,              {0,          63,         64,           127,        128,         191,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64B64_SFLOAT,            {0,          63,         64,           127,        128,         191,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64B64A64_UINT,           {0,          63,         64,           127,        128,         191,         192,          255,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64B64A64_SINT,           {0,          63,         64,           127,        128,         191,         192,          255,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::R64G64B64A64_SFLOAT,         {0,          63,         64,           127,        128,         191,         192,          255,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::B10G11R11_UFLOAT_PACK32,     {0,          10,         11,           21,         22,          31,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::E5B9G9R9_UFLOAT_PACK32,      {0,          8,          9,            17,         18,          26,          UINT32_MAX,   UINT32_MAX, 27,            31,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::D16_UNORM,                   {UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            15,         UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::X8_D24_UNORM_PACK32,         {UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            23,         UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::D32_SFLOAT,                  {UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            31,         UINT32_MAX,     UINT32_MAX} },
    {Anvil::Format::S8_UINT,                     {UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, 0,              7,        } },
    {Anvil::Format::D16_UNORM_S8_UINT,           {UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            15,         16,             23,       } },
    {Anvil::Format::D24_UNORM_S8_UINT,           {UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            23,         24,             31,       } },
    {Anvil::Format::D32_SFLOAT_S8_UINT,          {UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            31,         32,             39,       } },
};

static const uint32_t g_layout_to_n_components[] =
{
    /* COMPONENT_LAYOUT_ABGR */
    4,

    /* COMPONENT_LAYOUT_ARGB */
    4,

    /* COMPONENT_LAYOUT_B */
    1,

    /* COMPONENT_LAYOUT_BGR */
    3,

    /* COMPONENT_LAYOUT_BGRA */
    4,

    /* COMPONENT_LAYOUT_BGRG */
    4,

    /* COMPONENT_LAYOUT_BR */
    2,

    /* COMPONENT_LAYOUT_BX */
    1,

    /* COMPONENT_LAYOUT_BXGXRXGX */
    4,

    /* COMPONENT_LAYOUT_BXRX */
    2,

    /* COMPONENT_LAYOUT_D */
    1,

    /* COMPONENT_LAYOUT_DS */
    2,

    /* COMPONENT_LAYOUT_EBGR */
    4,

    /* COMPONENT_LAYOUT_G */
    1,

    /* COMPONENT_LAYOUT_GBGR */
    4,

    /* COMPONENT_LAYOUT_GX */
    1,

    /* COMPONENT_LAYOUT_GXBXGXRX */
    4,

    /* COMPONENT_LAYOUT_R */
    1,

    /* COMPONENT_LAYOUT_RG */
    2,

    /* COMPONENT_LAYOUT_RGB */
    3,

    /* COMPONENT_LAYOUT_RGBA */
    4,

    /* COMPONENT_LAYOUT_RX */
    1,

    /* COMPONENT_LAYOUT_RXGX */
    2,

    /* COMPONENT_LAYOUT_RXGXBXAX */
    4,

    /* COMPONENT_LAYOUT_S */
    1,

    /* COMPONENT_LAYOUT_XD */
    2,
};

static const std::vector<Anvil::Format> g_compatibility_classes[] =
{
    /* 8-bit */
    {
        Anvil::Format::R4G4_UNORM_PACK8,
        Anvil::Format::R8_UNORM,
        Anvil::Format::R8_SNORM,
        Anvil::Format::R8_USCALED,
        Anvil::Format::R8_SSCALED,
        Anvil::Format::R8_UINT,
        Anvil::Format::R8_SINT,
        Anvil::Format::R8_SRGB
    },

    {
        Anvil::Format::G8_B8_R8_3PLANE_420_UNORM
    },

    {
        Anvil::Format::G8_B8R8_2PLANE_420_UNORM
    },

    {
        Anvil::Format::G8_B8_R8_3PLANE_422_UNORM
    },

    {
        Anvil::Format::G8_B8R8_2PLANE_422_UNORM
    },

    {
        Anvil::Format::G8_B8_R8_3PLANE_444_UNORM
    },

    /* 10-bit */
    {
        Anvil::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16
    },

    {
        Anvil::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16
    },

    {
        Anvil::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16
    },

    {
        Anvil::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16
    },

    {
        Anvil::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16
    },

    /* 12-bit */

    {
        Anvil::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16
    },

    {
        Anvil::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16
    },

    {
        Anvil::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16
    },

    {
        Anvil::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16
    },

    {
        Anvil::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16
    },

    /* 16-bit */
    {
        Anvil::Format::R4G4B4A4_UNORM_PACK16,
        Anvil::Format::B4G4R4A4_UNORM_PACK16,
        Anvil::Format::R5G6B5_UNORM_PACK16,
        Anvil::Format::B5G6R5_UNORM_PACK16,
        Anvil::Format::R5G5B5A1_UNORM_PACK16,
        Anvil::Format::B5G5R5A1_UNORM_PACK16,
        Anvil::Format::A1R5G5B5_UNORM_PACK16,
        Anvil::Format::R8G8_UNORM,
        Anvil::Format::R8G8_SNORM,
        Anvil::Format::R8G8_USCALED,
        Anvil::Format::R8G8_SSCALED,
        Anvil::Format::R8G8_UINT,
        Anvil::Format::R8G8_SINT,
        Anvil::Format::R8G8_SRGB,
        Anvil::Format::R16_UNORM,
        Anvil::Format::R16_SNORM,
        Anvil::Format::R16_USCALED,
        Anvil::Format::R16_SSCALED,
        Anvil::Format::R16_UINT,
        Anvil::Format::R16_SINT,
        Anvil::Format::R16_SFLOAT,

        /* YUV */
        Anvil::Format::R10X6_UNORM_PACK16,
        Anvil::Format::R12X4_UNORM_PACK16
    },

    {
        Anvil::Format::G16_B16_R16_3PLANE_420_UNORM,
    },

    {
        Anvil::Format::G16_B16R16_2PLANE_420_UNORM,
    },

    {
        Anvil::Format::G16_B16_R16_3PLANE_422_UNORM,
    },

    {
        Anvil::Format::G16_B16R16_2PLANE_422_UNORM,
    },

    {
        Anvil::Format::G16_B16_R16_3PLANE_444_UNORM,
    },

    /* 24-bit */
    {
        Anvil::Format::R8G8B8_UNORM,
        Anvil::Format::R8G8B8_SNORM,
        Anvil::Format::R8G8B8_USCALED,
        Anvil::Format::R8G8B8_SSCALED,
        Anvil::Format::R8G8B8_UINT,
        Anvil::Format::R8G8B8_SINT,
        Anvil::Format::R8G8B8_SRGB,
        Anvil::Format::B8G8R8_UNORM,
        Anvil::Format::B8G8R8_SNORM,
        Anvil::Format::B8G8R8_USCALED,
        Anvil::Format::B8G8R8_SSCALED,
        Anvil::Format::B8G8R8_UINT,
        Anvil::Format::B8G8R8_SINT,
        Anvil::Format::B8G8R8_SRGB
    },

    /* 32-bit */
    {
        Anvil::Format::R8G8B8A8_UNORM,
        Anvil::Format::R8G8B8A8_SNORM,
        Anvil::Format::R8G8B8A8_USCALED,
        Anvil::Format::R8G8B8A8_SSCALED,
        Anvil::Format::R8G8B8A8_UINT,
        Anvil::Format::R8G8B8A8_SINT,
        Anvil::Format::R8G8B8A8_SRGB,
        Anvil::Format::B8G8R8A8_UNORM,
        Anvil::Format::B8G8R8A8_SNORM,
        Anvil::Format::B8G8R8A8_USCALED,
        Anvil::Format::B8G8R8A8_SSCALED,
        Anvil::Format::B8G8R8A8_UINT,
        Anvil::Format::B8G8R8A8_SINT,
        Anvil::Format::B8G8R8A8_SRGB,
        Anvil::Format::A8B8G8R8_UNORM_PACK32,
        Anvil::Format::A8B8G8R8_SNORM_PACK32,
        Anvil::Format::A8B8G8R8_USCALED_PACK32,
        Anvil::Format::A8B8G8R8_SSCALED_PACK32,
        Anvil::Format::A8B8G8R8_UINT_PACK32,
        Anvil::Format::A8B8G8R8_SINT_PACK32,
        Anvil::Format::A8B8G8R8_SRGB_PACK32,
        Anvil::Format::A2R10G10B10_UNORM_PACK32,
        Anvil::Format::A2R10G10B10_SNORM_PACK32,
        Anvil::Format::A2R10G10B10_USCALED_PACK32,
        Anvil::Format::A2R10G10B10_SSCALED_PACK32,
        Anvil::Format::A2R10G10B10_UINT_PACK32,
        Anvil::Format::A2R10G10B10_SINT_PACK32,
        Anvil::Format::A2B10G10R10_UNORM_PACK32,
        Anvil::Format::A2B10G10R10_SNORM_PACK32,
        Anvil::Format::A2B10G10R10_USCALED_PACK32,
        Anvil::Format::A2B10G10R10_SSCALED_PACK32,
        Anvil::Format::A2B10G10R10_UINT_PACK32,
        Anvil::Format::A2B10G10R10_SINT_PACK32,
        Anvil::Format::R16G16_UNORM,
        Anvil::Format::R16G16_SNORM,
        Anvil::Format::R16G16_USCALED,
        Anvil::Format::R16G16_SSCALED,
        Anvil::Format::R16G16_UINT,
        Anvil::Format::R16G16_SINT,
        Anvil::Format::R16G16_SFLOAT,
        Anvil::Format::R32_UINT,
        Anvil::Format::R32_SINT,
        Anvil::Format::R32_SFLOAT,
        Anvil::Format::B10G11R11_UFLOAT_PACK32,
        Anvil::Format::E5B9G9R9_UFLOAT_PACK32,

        /* YUV */
        Anvil::Format::R10X6G10X6_UNORM_2PACK16,
        Anvil::Format::R12X4G12X4_UNORM_2PACK16,
    },

    {
        Anvil::Format::G8B8G8R8_422_UNORM,
    },

    {
        Anvil::Format::B8G8R8G8_422_UNORM
    },

    /* 48-bit */
    {
        Anvil::Format::R16G16B16_UNORM,
        Anvil::Format::R16G16B16_SNORM,
        Anvil::Format::R16G16B16_USCALED,
        Anvil::Format::R16G16B16_SSCALED,
        Anvil::Format::R16G16B16_UINT,
        Anvil::Format::R16G16B16_SINT,
        Anvil::Format::R16G16B16_SFLOAT
    },

    /* 64-bit */
    {
        Anvil::Format::R16G16B16A16_UNORM,
        Anvil::Format::R16G16B16A16_SNORM,
        Anvil::Format::R16G16B16A16_USCALED,
        Anvil::Format::R16G16B16A16_SSCALED,
        Anvil::Format::R16G16B16A16_UINT,
        Anvil::Format::R16G16B16A16_SINT,
        Anvil::Format::R16G16B16A16_SFLOAT,
        Anvil::Format::R32G32_UINT,
        Anvil::Format::R32G32_SINT,
        Anvil::Format::R32G32_SFLOAT,
        Anvil::Format::R64_UINT,
        Anvil::Format::R64_SINT,
        Anvil::Format::R64_SFLOAT,
    },

    {
        Anvil::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16,
    },

    {
        Anvil::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
    },

    {
        Anvil::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
    },

    {
        Anvil::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16,
    },

    {
        Anvil::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
    },

    {
        Anvil::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
    },

    {
        Anvil::Format::G16B16G16R16_422_UNORM,
    },

    {
        Anvil::Format::B16G16R16G16_422_UNORM
    },

    /* 96-bit */
    {
        Anvil::Format::R32G32B32_UINT,
        Anvil::Format::R32G32B32_SINT,
        Anvil::Format::R32G32B32_SFLOAT
    },

    /* 128-bit */
    {
        Anvil::Format::R32G32B32A32_UINT,
        Anvil::Format::R32G32B32A32_SINT,
        Anvil::Format::R32G32B32A32_SFLOAT,
        Anvil::Format::R64G64_UINT,
        Anvil::Format::R64G64_SINT,
        Anvil::Format::R64G64_SFLOAT
    },

    /* 192-bit */
    {
        Anvil::Format::R64G64B64_UINT,
        Anvil::Format::R64G64B64_SINT,
        Anvil::Format::R64G64B64_SFLOAT
    },

    /* 256-bit */
    {
        Anvil::Format::R64G64B64A64_UINT,
        Anvil::Format::R64G64B64A64_SINT,
        Anvil::Format::R64G64B64A64_SFLOAT
    },

    /* BC1 RGB  */
    {
        Anvil::Format::BC1_RGB_UNORM_BLOCK,
        Anvil::Format::BC1_RGB_SRGB_BLOCK
    },

    /* BC1 RGBA */
    {
        Anvil::Format::BC1_RGBA_UNORM_BLOCK,
        Anvil::Format::BC1_RGBA_SRGB_BLOCK
    },

    /* BC2 */
    {
        Anvil::Format::BC2_UNORM_BLOCK,
        Anvil::Format::BC2_SRGB_BLOCK
    },

    /* BC3 */
    {
        Anvil::Format::BC3_UNORM_BLOCK,
        Anvil::Format::BC3_SRGB_BLOCK
    },

    /* BC4 */
    {
        Anvil::Format::BC4_UNORM_BLOCK,
        Anvil::Format::BC4_SNORM_BLOCK
    },

    /* BC5 */
    {
        Anvil::Format::BC5_UNORM_BLOCK,
        Anvil::Format::BC5_SNORM_BLOCK
    },

    /* BC6H */
    {
        Anvil::Format::BC6H_UFLOAT_BLOCK,
        Anvil::Format::BC6H_SFLOAT_BLOCK
    },

    /* BC7 */
    {
        Anvil::Format::BC7_UNORM_BLOCK,
        Anvil::Format::BC7_SRGB_BLOCK
    },

    /* ETC2 RGB */
    {
        Anvil::Format::ETC2_R8G8B8_UNORM_BLOCK,
        Anvil::Format::ETC2_R8G8B8_SRGB_BLOCK
    },

    /* ETC2 RGBA */
    {
        Anvil::Format::ETC2_R8G8B8A1_UNORM_BLOCK,
        Anvil::Format::ETC2_R8G8B8A1_SRGB_BLOCK
    },

    /* ETC2/EAC RGBA */
    {
        Anvil::Format::ETC2_R8G8B8A8_UNORM_BLOCK,
        Anvil::Format::ETC2_R8G8B8A8_SRGB_BLOCK
    },

    /* EAC R */
    {
        Anvil::Format::EAC_R11_UNORM_BLOCK,
        Anvil::Format::EAC_R11_SNORM_BLOCK
    },

    /* EAC RG */
    {
        Anvil::Format::EAC_R11G11_UNORM_BLOCK,
        Anvil::Format::EAC_R11G11_SNORM_BLOCK
    },

    /* ASTC (4x4) */
    {
        Anvil::Format::ASTC_4x4_UNORM_BLOCK,
        Anvil::Format::ASTC_4x4_SRGB_BLOCK
    },

    /* ASTC (5x4) */
    {
        Anvil::Format::ASTC_5x4_UNORM_BLOCK,
        Anvil::Format::ASTC_5x4_SRGB_BLOCK
    },

    /* ASTC (5x5) */
    {
        Anvil::Format::ASTC_5x5_UNORM_BLOCK,
        Anvil::Format::ASTC_5x5_SRGB_BLOCK
    },

    /* ASTC (6x5) */
    {
        Anvil::Format::ASTC_6x5_UNORM_BLOCK,
        Anvil::Format::ASTC_6x5_SRGB_BLOCK
    },

    /* ASTC (6x6) */
    {
        Anvil::Format::ASTC_6x6_UNORM_BLOCK,
        Anvil::Format::ASTC_6x6_SRGB_BLOCK
    },

    /* ASTC (8x5) */
    {
        Anvil::Format::ASTC_8x5_UNORM_BLOCK,
        Anvil::Format::ASTC_8x5_SRGB_BLOCK
    },

    /* ASTC (8x6) */
    {
        Anvil::Format::ASTC_8x6_UNORM_BLOCK,
        Anvil::Format::ASTC_8x6_SRGB_BLOCK
    },

    /* ASTC (8x8) */
    {
        Anvil::Format::ASTC_8x8_UNORM_BLOCK,
        Anvil::Format::ASTC_8x8_SRGB_BLOCK
    },

    /* ASTC (10x5) */
    {
        Anvil::Format::ASTC_10x5_UNORM_BLOCK,
        Anvil::Format::ASTC_10x5_SRGB_BLOCK
    },

    /* ASTC (10x6) */
    {
        Anvil::Format::ASTC_10x6_UNORM_BLOCK,
        Anvil::Format::ASTC_10x6_SRGB_BLOCK
    },

    /* ASTC (10x8) */
    {
        Anvil::Format::ASTC_10x8_UNORM_BLOCK,
        Anvil::Format::ASTC_10x8_SRGB_BLOCK
    },

    /* ASTC (10x10) */
    {
        Anvil::Format::ASTC_10x10_UNORM_BLOCK,
        Anvil::Format::ASTC_10x10_SRGB_BLOCK
    },

    /* ASTC (12x10) */
    {
        Anvil::Format::ASTC_12x10_UNORM_BLOCK,
        Anvil::Format::ASTC_12x10_SRGB_BLOCK
    },

    /* ASTC (12x12) */
    {
        Anvil::Format::ASTC_12x12_UNORM_BLOCK,
        Anvil::Format::ASTC_12x12_SRGB_BLOCK
    },

    /* D16 */
    {
        Anvil::Format::D16_UNORM
    },

    /* D24 */
    {
        Anvil::Format::X8_D24_UNORM_PACK32
    },

    /* D32 */
    {
        Anvil::Format::D32_SFLOAT
    },

    /* S8 */
    {
        Anvil::Format::S8_UINT
    },

    /* D16S8 */
    {
        Anvil::Format::D16_UNORM_S8_UINT
    },

    /* D24S8 */
    {
        Anvil::Format::D24_UNORM_S8_UINT
    },

    /* D32S8 */
    {
        Anvil::Format::D32_SFLOAT_S8_UINT
    }
};

/** Please see header for specification */
bool Anvil::Formats::get_compatible_formats(Anvil::Format         in_format,
                                            uint32_t*             out_n_compatible_formats_ptr,
                                            const Anvil::Format** out_compatible_formats_ptr_ptr)
{
    bool result = false;

    for (const auto& current_class : g_compatibility_classes)
    {
        auto format_iterator = std::find(current_class.begin(),
                                         current_class.end  (),
                                         in_format);

        if (format_iterator != current_class.end() )
        {
            *out_n_compatible_formats_ptr   = static_cast<uint32_t>(current_class.size() );
            *out_compatible_formats_ptr_ptr = &current_class.at(0);

            result = true;
            break;
        }
    }

    return result;
}

/** Please see header for specification */
bool Anvil::Formats::get_compressed_format_block_size(Anvil::Format in_format,
                                                      uint32_t*     out_block_size_uvec2_ptr,
                                                      uint32_t*     out_n_bytes_per_block_ptr)
{
    bool result = true;

    switch (in_format)
    {
        case Anvil::Format::ASTC_4x4_UNORM_BLOCK:
        case Anvil::Format::ASTC_4x4_SRGB_BLOCK:
        case Anvil::Format::BC2_UNORM_BLOCK:
        case Anvil::Format::BC2_SRGB_BLOCK:
        case Anvil::Format::BC3_UNORM_BLOCK:
        case Anvil::Format::BC3_SRGB_BLOCK:
        case Anvil::Format::BC5_UNORM_BLOCK:
        case Anvil::Format::BC5_SNORM_BLOCK:
        case Anvil::Format::BC6H_UFLOAT_BLOCK:
        case Anvil::Format::BC6H_SFLOAT_BLOCK:
        case Anvil::Format::BC7_UNORM_BLOCK:
        case Anvil::Format::BC7_SRGB_BLOCK:
        case Anvil::Format::EAC_R11G11_UNORM_BLOCK:
        case Anvil::Format::EAC_R11G11_SNORM_BLOCK:
        case Anvil::Format::ETC2_R8G8B8A8_UNORM_BLOCK:
        case Anvil::Format::ETC2_R8G8B8A8_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 4;
             out_block_size_uvec2_ptr[1] = 4;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::BC1_RGB_UNORM_BLOCK:
        case Anvil::Format::BC1_RGB_SRGB_BLOCK:
        case Anvil::Format::BC1_RGBA_UNORM_BLOCK:
        case Anvil::Format::BC1_RGBA_SRGB_BLOCK:
        case Anvil::Format::BC4_UNORM_BLOCK:
        case Anvil::Format::BC4_SNORM_BLOCK:
        case Anvil::Format::EAC_R11_UNORM_BLOCK:
        case Anvil::Format::EAC_R11_SNORM_BLOCK:
        case Anvil::Format::ETC2_R8G8B8_UNORM_BLOCK:
        case Anvil::Format::ETC2_R8G8B8_SRGB_BLOCK:
        case Anvil::Format::ETC2_R8G8B8A1_UNORM_BLOCK:
        case Anvil::Format::ETC2_R8G8B8A1_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 4;
             out_block_size_uvec2_ptr[1] = 4;
            *out_n_bytes_per_block_ptr   = 64 / 8;

            break;
        }

        case Anvil::Format::ASTC_5x4_UNORM_BLOCK:
        case Anvil::Format::ASTC_5x4_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 5;
             out_block_size_uvec2_ptr[1] = 4;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_5x5_UNORM_BLOCK:
        case Anvil::Format::ASTC_5x5_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 5;
             out_block_size_uvec2_ptr[1] = 5;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_6x5_UNORM_BLOCK:
        case Anvil::Format::ASTC_6x5_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 6;
             out_block_size_uvec2_ptr[1] = 5;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_6x6_UNORM_BLOCK:
        case Anvil::Format::ASTC_6x6_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 6;
             out_block_size_uvec2_ptr[1] = 6;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_8x5_UNORM_BLOCK:
        case Anvil::Format::ASTC_8x5_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 8;
             out_block_size_uvec2_ptr[1] = 5;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_8x6_UNORM_BLOCK:
        case Anvil::Format::ASTC_8x6_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 8;
             out_block_size_uvec2_ptr[1] = 6;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_8x8_UNORM_BLOCK:
        case Anvil::Format::ASTC_8x8_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 8;
             out_block_size_uvec2_ptr[1] = 8;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_10x5_UNORM_BLOCK:
        case Anvil::Format::ASTC_10x5_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 10;
             out_block_size_uvec2_ptr[1] = 5;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_10x6_UNORM_BLOCK:
        case Anvil::Format::ASTC_10x6_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 10;
             out_block_size_uvec2_ptr[1] = 6;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_10x8_UNORM_BLOCK:
        case Anvil::Format::ASTC_10x8_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 10;
             out_block_size_uvec2_ptr[1] = 8;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_10x10_UNORM_BLOCK:
        case Anvil::Format::ASTC_10x10_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 10;
             out_block_size_uvec2_ptr[1] = 10;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_12x10_UNORM_BLOCK:
        case Anvil::Format::ASTC_12x10_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 12;
             out_block_size_uvec2_ptr[1] = 10;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        case Anvil::Format::ASTC_12x12_UNORM_BLOCK:
        case Anvil::Format::ASTC_12x12_SRGB_BLOCK:
        {
             out_block_size_uvec2_ptr[0] = 12;
             out_block_size_uvec2_ptr[1] = 12;
            *out_n_bytes_per_block_ptr   = 128 / 8;

            break;
        }

        // YUV KHR compressed formats
        case Anvil::Format::G8B8G8R8_422_UNORM:
        case Anvil::Format::B8G8R8G8_422_UNORM:
        {
            out_block_size_uvec2_ptr[0] = 2;
            out_block_size_uvec2_ptr[1] = 1;
           *out_n_bytes_per_block_ptr   = 32 / 8;

            break;
        }

        case Anvil::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
        case Anvil::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
        case Anvil::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
        case Anvil::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
        case Anvil::Format::G16B16G16R16_422_UNORM:
        case Anvil::Format::B16G16R16G16_422_UNORM:
        {
            out_block_size_uvec2_ptr[0] = 2;
            out_block_size_uvec2_ptr[1] = 1;
           *out_n_bytes_per_block_ptr   = 64 / 8;

            break;
        }

        default:
        {
            anvil_assert_fail();
        }
    }

    return result;
}

/** Please see header for specification */
Anvil::Format Anvil::Formats::get_format(ComponentLayout in_component_layout,
                                         FormatType      in_format_type,
                                         uint32_t        in_n_component0_bits,
                                         uint32_t        in_n_component1_bits,
                                         uint32_t        in_n_component2_bits,
                                         uint32_t        in_n_component3_bits)
{
    static const uint32_t n_available_formats = sizeof(g_formats) / sizeof(g_formats[0]);
    Anvil::Format         result              = Anvil::Format::UNKNOWN;

    for (uint32_t n_format = 0;
                  n_format < n_available_formats;
                ++n_format)
    {
        const FormatInfo& current_format_info = g_formats[n_format];

        if (current_format_info.component_layout  == in_component_layout  &&
            current_format_info.format_type       == in_format_type       &&
            current_format_info.component_bits[0] == in_n_component0_bits &&
            current_format_info.component_bits[1] == in_n_component1_bits &&
            current_format_info.component_bits[2] == in_n_component2_bits &&
            current_format_info.component_bits[3] == in_n_component3_bits)
        {
            result = current_format_info.format;

            break;
        }
    }

    return result;
}

/** Please see header for specification */
bool Anvil::Formats::get_format_aspects(Anvil::Format                         in_format,
                                        std::vector<Anvil::ImageAspectFlags>* out_aspects_ptr)
{
    bool result = false;

    out_aspects_ptr->clear();

    if (Anvil::Formats::is_format_yuv_khr(in_format) )
    {
        if (!Anvil::Formats::is_format_multiplanar(in_format) )
        {
            out_aspects_ptr->push_back(Anvil::ImageAspectFlagBits::COLOR_BIT);
        }
        else
        {
            const auto n_planes = get_format_n_planes(in_format);

            out_aspects_ptr->push_back(Anvil::ImageAspectFlagBits::PLANE_0_BIT);

            if (n_planes >= 2)
            {
                out_aspects_ptr->push_back(Anvil::ImageAspectFlagBits::PLANE_1_BIT);
            }

            if (n_planes >= 3)
            {
                out_aspects_ptr->push_back(Anvil::ImageAspectFlagBits::PLANE_2_BIT);
            }

            anvil_assert(n_planes >= 1 && n_planes <= 3);
        }
    }
    else
    {
        /* This image can hold color and/or depth and/or stencil aspects only */
        const Anvil::ComponentLayout format_layout = Anvil::Formats::get_format_component_layout_nonyuv(in_format);

        if (format_layout == Anvil::ComponentLayout::ABGR ||
            format_layout == Anvil::ComponentLayout::ARGB ||
            format_layout == Anvil::ComponentLayout::BGR  ||
            format_layout == Anvil::ComponentLayout::BGRA ||
            format_layout == Anvil::ComponentLayout::EBGR ||
            format_layout == Anvil::ComponentLayout::R    ||
            format_layout == Anvil::ComponentLayout::RG   ||
            format_layout == Anvil::ComponentLayout::RGB  ||
            format_layout == Anvil::ComponentLayout::RGBA)
        {
            out_aspects_ptr->push_back(Anvil::ImageAspectFlagBits::COLOR_BIT);
        }

        if (format_layout == Anvil::ComponentLayout::D  ||
            format_layout == Anvil::ComponentLayout::DS ||
            format_layout == Anvil::ComponentLayout::XD)
        {
            out_aspects_ptr->push_back(Anvil::ImageAspectFlagBits::DEPTH_BIT);
        }

        if (format_layout == Anvil::ComponentLayout::DS ||
            format_layout == Anvil::ComponentLayout::S)
        {
            out_aspects_ptr->push_back(Anvil::ImageAspectFlagBits::STENCIL_BIT);
        }
    }

    result = true;
    return result;
}

/** Please see header for specification */
void Anvil::Formats::get_format_bit_layout_nonyuv(Anvil::Format in_format,
                                                  uint32_t*     out_opt_red_component_start_bit_index_ptr,
                                                  uint32_t*     out_opt_red_component_end_bit_index_ptr,
                                                  uint32_t*     out_opt_green_component_start_bit_index_ptr,
                                                  uint32_t*     out_opt_green_component_end_bit_index_ptr,
                                                  uint32_t*     out_opt_blue_component_start_bit_index_ptr,
                                                  uint32_t*     out_opt_blue_component_end_bit_index_ptr,
                                                  uint32_t*     out_opt_alpha_component_start_bit_index_ptr,
                                                  uint32_t*     out_opt_alpha_component_end_bit_index_ptr,
                                                  uint32_t*     out_opt_shared_component_start_bit_index_ptr,
                                                  uint32_t*     out_opt_shared_component_end_bit_index_ptr,
                                                  uint32_t*     out_opt_depth_component_start_bit_index_ptr,
                                                  uint32_t*     out_opt_depth_component_end_bit_index_ptr,
                                                  uint32_t*     out_opt_stencil_component_start_bit_index_ptr,
                                                  uint32_t*     out_opt_stencil_component_end_bit_index_ptr)
{
    anvil_assert(!is_format_yuv_khr(in_format) );

    const auto& format_info = g_nonyuv_format_bit_layout_info.at(in_format);

    if (out_opt_red_component_start_bit_index_ptr != nullptr)
    {
        *out_opt_red_component_start_bit_index_ptr = format_info.red_component_start_bit_index;
    }

    if (out_opt_red_component_end_bit_index_ptr != nullptr)
    {
        *out_opt_red_component_end_bit_index_ptr = format_info.red_component_last_bit_index;
    }

    if (out_opt_green_component_start_bit_index_ptr != nullptr)
    {
        *out_opt_green_component_start_bit_index_ptr = format_info.green_component_start_bit_index;
    }

    if (out_opt_green_component_end_bit_index_ptr != nullptr)
    {
        *out_opt_green_component_end_bit_index_ptr = format_info.green_component_last_bit_index;
    }

    if (out_opt_blue_component_start_bit_index_ptr != nullptr)
    {
        *out_opt_blue_component_start_bit_index_ptr = format_info.blue_component_start_bit_index;
    }

    if (out_opt_blue_component_end_bit_index_ptr != nullptr)
    {
        *out_opt_blue_component_end_bit_index_ptr = format_info.blue_component_last_bit_index;
    }

    if (out_opt_alpha_component_start_bit_index_ptr != nullptr)
    {
        *out_opt_alpha_component_start_bit_index_ptr = format_info.alpha_component_start_bit_index;
    }

    if (out_opt_alpha_component_end_bit_index_ptr != nullptr)
    {
        *out_opt_alpha_component_end_bit_index_ptr = format_info.alpha_component_last_bit_index;
    }

    if (out_opt_shared_component_start_bit_index_ptr != nullptr)
    {
        *out_opt_shared_component_start_bit_index_ptr = format_info.shared_component_start_bit_index;
    }

    if (out_opt_shared_component_end_bit_index_ptr != nullptr)
    {
        *out_opt_shared_component_end_bit_index_ptr = format_info.shared_component_last_bit_index;
    }

    if (out_opt_depth_component_start_bit_index_ptr != nullptr)
    {
        *out_opt_depth_component_start_bit_index_ptr = format_info.depth_component_start_bit_index;
    }

    if (out_opt_depth_component_end_bit_index_ptr != nullptr)
    {
        *out_opt_depth_component_end_bit_index_ptr = format_info.depth_component_last_bit_index;
    }

    if (out_opt_stencil_component_start_bit_index_ptr != nullptr)
    {
        *out_opt_stencil_component_start_bit_index_ptr = format_info.stencil_component_start_bit_index;
    }

    if (out_opt_stencil_component_end_bit_index_ptr != nullptr)
    {
        *out_opt_stencil_component_end_bit_index_ptr = format_info.stencil_component_last_bit_index;
    }
}

void Anvil::Formats::get_format_bit_layout_yuv(Anvil::Format in_format,
                                               uint32_t*     out_opt_plane0_r0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_r0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_g0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_g0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_b0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_b0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_a0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_a0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_g1_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane0_g1_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane1_r0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane1_r0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane1_g0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane1_g0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane1_b0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane1_b0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane2_r0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane2_r0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane2_g0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane2_g0_end_bit_index_ptr,
                                               uint32_t*     out_opt_plane2_b0_start_bit_index_ptr,
                                               uint32_t*     out_opt_plane2_b0_end_bit_index_ptr)
{
    anvil_assert(is_format_yuv_khr(in_format) );

    const auto& format_info = g_yuv_format_bit_layout_info.at(in_format);

    if (out_opt_plane0_r0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_r0_start_bit_index_ptr = format_info.plane0_r0_start_bit_index;
    }

    if (out_opt_plane0_r0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_r0_end_bit_index_ptr = format_info.plane0_r0_last_bit_index;
    }

    if (out_opt_plane0_g0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_g0_start_bit_index_ptr = format_info.plane0_g0_start_bit_index;
    }

    if (out_opt_plane0_g0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_g0_end_bit_index_ptr = format_info.plane0_g0_last_bit_index;
    }

    if (out_opt_plane0_b0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_b0_start_bit_index_ptr = format_info.plane0_b0_start_bit_index;
    }

    if (out_opt_plane0_b0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_b0_end_bit_index_ptr = format_info.plane0_b0_last_bit_index;
    }

    if (out_opt_plane0_a0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_a0_start_bit_index_ptr = format_info.plane0_a0_start_bit_index;
    }

    if (out_opt_plane0_a0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_a0_end_bit_index_ptr = format_info.plane0_a0_last_bit_index;
    }

    if (out_opt_plane0_g1_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_g1_start_bit_index_ptr = format_info.plane0_g0_start_bit_index;
    }

    if (out_opt_plane0_g1_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane0_g1_end_bit_index_ptr = format_info.plane0_g1_last_bit_index;
    }

    if (out_opt_plane1_r0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane1_r0_start_bit_index_ptr = format_info.plane1_r0_start_bit_index;
    }

    if (out_opt_plane1_r0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane1_r0_end_bit_index_ptr = format_info.plane1_r0_last_bit_index;
    }

    if (out_opt_plane1_g0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane1_g0_start_bit_index_ptr = format_info.plane1_g0_start_bit_index;
    }

    if (out_opt_plane1_g0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane1_g0_end_bit_index_ptr = format_info.plane1_g0_last_bit_index;
    }

    if (out_opt_plane1_b0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane1_b0_start_bit_index_ptr = format_info.plane1_b0_start_bit_index;
    }

    if (out_opt_plane1_b0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane1_b0_end_bit_index_ptr = format_info.plane1_b0_last_bit_index;
    }

    if (out_opt_plane2_r0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane2_r0_start_bit_index_ptr = format_info.plane2_r0_start_bit_index;
    }

    if (out_opt_plane2_r0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane2_r0_end_bit_index_ptr = format_info.plane2_r0_last_bit_index;
    }

    if (out_opt_plane2_g0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane2_g0_start_bit_index_ptr = format_info.plane2_g0_start_bit_index;
    }

    if (out_opt_plane2_g0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane2_g0_end_bit_index_ptr = format_info.plane2_g0_last_bit_index;
    }

    if (out_opt_plane2_b0_start_bit_index_ptr != nullptr)
    {
        *out_opt_plane2_b0_start_bit_index_ptr = format_info.plane2_b0_start_bit_index;
    }

    if (out_opt_plane2_b0_end_bit_index_ptr != nullptr)
    {
        *out_opt_plane2_b0_end_bit_index_ptr = format_info.plane2_b0_last_bit_index;
    }
}

/** Please see header for specification */
Anvil::ComponentLayout Anvil::Formats::get_format_component_layout_nonyuv(Anvil::Format in_format)
{
    static_assert(sizeof(g_formats) / sizeof(g_formats[0]) == VK_FORMAT_RANGE_SIZE, "");
    anvil_assert(static_cast<uint32_t>(in_format)          <  VK_FORMAT_RANGE_SIZE);
    anvil_assert(!Anvil::Formats::is_format_yuv_khr(in_format) );

    return g_formats[static_cast<uint32_t>(in_format)].component_layout;
}

/** Please see header for specification */
Anvil::ComponentLayout Anvil::Formats::get_format_component_layout_yuv(Anvil::Format              in_format,
                                                                       Anvil::ImageAspectFlagBits in_aspect)
{
    const auto plane_idx           = Anvil::Formats::get_yuv_format_plane_index(in_format,
                                                                                in_aspect);
    const auto YUV_FORMAT_KHR_SIZE = static_cast<uint32_t>(Anvil::Format::G16_B16_R16_3PLANE_444_UNORM) - static_cast<uint32_t>(Anvil::Format::G8B8G8R8_422_UNORM) + 1;

    ANVIL_REDUNDANT_VARIABLE_CONST(YUV_FORMAT_KHR_SIZE);

    anvil_assert(g_yuv_formats.size() == YUV_FORMAT_KHR_SIZE);
    anvil_assert(Anvil::Formats::is_format_yuv_khr(in_format) );

    return g_yuv_formats.at(in_format).subresources[plane_idx].component_layout;
}

/** Please see header for specification */
uint32_t Anvil::Formats::get_format_n_components_nonyuv(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE);
    anvil_assert(!Anvil::Formats::is_format_yuv_khr(in_format) );

    return g_layout_to_n_components[static_cast<uint32_t>(g_formats[static_cast<uint32_t>(in_format)].component_layout)];
}

/** Please see header for specification */
uint32_t Anvil::Formats::get_format_n_components_yuv(Anvil::Format              in_format,
                                                     Anvil::ImageAspectFlagBits in_aspect)
{
    const auto plane_idx = Anvil::Formats::get_yuv_format_plane_index(in_format,
                                                                      in_aspect);

    anvil_assert(Anvil::Formats::is_format_yuv_khr(in_format) );

    return g_layout_to_n_components[static_cast<uint32_t>(g_yuv_formats.at(in_format).subresources[plane_idx].component_layout)];
}

/** Please see header for specification */
void Anvil::Formats::get_format_n_component_bits_nonyuv(Anvil::Format in_format,
                                                        uint32_t*     out_channel0_bits_ptr,
                                                        uint32_t*     out_channel1_bits_ptr,
                                                        uint32_t*     out_channel2_bits_ptr,
                                                        uint32_t*     out_channel3_bits_ptr)
{
    const FormatInfo* format_props_ptr = nullptr;

    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE);
    anvil_assert(!Anvil::Formats::is_format_yuv_khr(in_format) );

    format_props_ptr = g_formats + static_cast<uint32_t>(in_format);

    *out_channel0_bits_ptr = format_props_ptr->component_bits[0];
    *out_channel1_bits_ptr = format_props_ptr->component_bits[1];
    *out_channel2_bits_ptr = format_props_ptr->component_bits[2];
    *out_channel3_bits_ptr = format_props_ptr->component_bits[3];
}

/** Please see header for specification */
void Anvil::Formats::get_format_n_component_bits_yuv(Anvil::Format              in_format,
                                                     Anvil::ImageAspectFlagBits in_aspect,
                                                     uint32_t*                  out_channel0_bits_ptr,
                                                     uint32_t*                  out_channel1_bits_ptr,
                                                     uint32_t*                  out_channel2_bits_ptr,
                                                     uint32_t*                  out_channel3_bits_ptr)
{
    const SubresourceLayoutInfo* format_props_ptr = nullptr;
    const auto                   plane_idx        = Anvil::Formats::get_yuv_format_plane_index(in_format,
                                                                                               in_aspect);

    anvil_assert(Anvil::Formats::is_format_yuv_khr(in_format) );

    format_props_ptr = &g_yuv_formats.at(in_format).subresources.at(plane_idx);

    *out_channel0_bits_ptr = format_props_ptr->component_bits_used[0];
    *out_channel1_bits_ptr = format_props_ptr->component_bits_used[1];
    *out_channel2_bits_ptr = format_props_ptr->component_bits_used[2];
    *out_channel3_bits_ptr = format_props_ptr->component_bits_used[3];
}

/** Please see header for specification */
uint32_t Anvil::Formats::get_format_n_planes(Anvil::Format in_format)
{
    uint32_t result = 1;

    switch (in_format)
    {
        case Anvil::Format::G8_B8R8_2PLANE_420_UNORM:
        case Anvil::Format::G8_B8R8_2PLANE_422_UNORM:
        case Anvil::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
        case Anvil::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
        case Anvil::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
        case Anvil::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
        case Anvil::Format::G16_B16R16_2PLANE_420_UNORM:
        case Anvil::Format::G16_B16R16_2PLANE_422_UNORM:
        {
            result = 2;

            break;
        }

        case Anvil::Format::G8_B8_R8_3PLANE_420_UNORM:
        case Anvil::Format::G8_B8_R8_3PLANE_422_UNORM:
        case Anvil::Format::G8_B8_R8_3PLANE_444_UNORM:
        case Anvil::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
        case Anvil::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
        case Anvil::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
        case Anvil::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
        case Anvil::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
        case Anvil::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
        case Anvil::Format::G16_B16_R16_3PLANE_420_UNORM:
        case Anvil::Format::G16_B16_R16_3PLANE_422_UNORM:
        case Anvil::Format::G16_B16_R16_3PLANE_444_UNORM:
        {
            result = 3;

            break;
        }

        default:
        {
            /* Fall-through */
        }
    }

    return result;
}

/** Please see header for specification */
void Anvil::Formats::get_format_n_unused_component_bits_yuv(Anvil::Format              in_format,
                                                            Anvil::ImageAspectFlagBits in_aspect,
                                                            uint32_t*                  out_channel0_unused_bits_ptr,
                                                            uint32_t*                  out_channel1_unused_bits_ptr,
                                                            uint32_t*                  out_channel2_unused_bits_ptr,
                                                            uint32_t*                  out_channel3_unused_bits_ptr)
{
    const SubresourceLayoutInfo* format_props_ptr = nullptr;
    const auto                   plane_idx        = Anvil::Formats::get_yuv_format_plane_index(in_format,
                                                                                               in_aspect);

    anvil_assert(Anvil::Formats::is_format_yuv_khr(in_format) );

    format_props_ptr = &g_yuv_formats.at(in_format).subresources.at(plane_idx);

    *out_channel0_unused_bits_ptr = format_props_ptr->component_bits_unused[0];
    *out_channel1_unused_bits_ptr = format_props_ptr->component_bits_unused[1];
    *out_channel2_unused_bits_ptr = format_props_ptr->component_bits_unused[2];
    *out_channel3_unused_bits_ptr = format_props_ptr->component_bits_unused[3];
}

/** Please see header for specification */
const char* Anvil::Formats::get_format_name(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE || Anvil::Formats::is_format_yuv_khr(in_format) );

    if (Anvil::Formats::is_format_yuv_khr(in_format) )
    {
        return g_yuv_formats.at(in_format).name;
    }
    else
    {
        return g_formats[static_cast<uint32_t>(in_format)].name;
    }
}

/** Please see header for specification */
Anvil::FormatType Anvil::Formats::get_format_type(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE || Anvil::Formats::is_format_yuv_khr(in_format) );

    if (Anvil::Formats::is_format_yuv_khr(in_format) )
    {
        return g_yuv_formats.at(in_format).format_type;
    }
    else
    {
        return g_formats[static_cast<uint32_t>(in_format)].format_type;
    }
}

/** Please see header for specification */
uint32_t Anvil::Formats::get_yuv_format_n_planes(Anvil::Format in_format)
{
    anvil_assert(Anvil::Formats::is_format_yuv_khr(in_format) );

    return g_yuv_formats.at(in_format).num_planes;
}

/** Please see header for specification */
void Anvil::Formats::get_yuv_format_plane_extent(Anvil::Format              in_format,
                                                 Anvil::ImageAspectFlagBits in_aspect,
                                                 VkExtent3D                 in_image_extent,
                                                 VkExtent3D*                out_plane_extent_ptr)
{
    anvil_assert(Anvil::Formats::is_format_yuv_khr(in_format) );

    if (!Anvil::Formats::is_format_multiplanar(in_format) )
    {
        anvil_assert(in_aspect == Anvil::ImageAspectFlagBits::COLOR_BIT);

        switch (in_format)
        {
            case Anvil::Format::G8B8G8R8_422_UNORM:
            case Anvil::Format::B8G8R8G8_422_UNORM:
            case Anvil::Format::G16B16G16R16_422_UNORM:
            case Anvil::Format::B16G16R16G16_422_UNORM:
            case Anvil::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
            case Anvil::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
            case Anvil::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
            case Anvil::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
            {
                anvil_assert((in_image_extent.width % 2) == 0);

               *out_plane_extent_ptr = in_image_extent;
                break;
            }

            case Anvil::Format::R10X6_UNORM_PACK16:
            case Anvil::Format::R12X4_UNORM_PACK16:
            case Anvil::Format::R10X6G10X6_UNORM_2PACK16:
            case Anvil::Format::R12X4G12X4_UNORM_2PACK16:
            case Anvil::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16:
            case Anvil::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16:
            {
                *out_plane_extent_ptr = in_image_extent;
                break;
            }

            default:
            {
                anvil_assert_fail();
            }
        }
    }
    else
    {
        anvil_assert(in_aspect == Anvil::ImageAspectFlagBits::PLANE_0_BIT ||
                     in_aspect == Anvil::ImageAspectFlagBits::PLANE_1_BIT ||
                     in_aspect == Anvil::ImageAspectFlagBits::PLANE_2_BIT);

        switch (in_format)
        {
            case Anvil::Format::G8_B8_R8_3PLANE_420_UNORM:
            case Anvil::Format::G8_B8R8_2PLANE_420_UNORM:
            case Anvil::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
            case Anvil::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
            case Anvil::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
            case Anvil::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
            case Anvil::Format::G16_B16_R16_3PLANE_420_UNORM:
            case Anvil::Format::G16_B16R16_2PLANE_420_UNORM:
            {
                anvil_assert((in_image_extent.width  % 2) == 0);
                anvil_assert((in_image_extent.height % 2) == 0);

                if (in_aspect == Anvil::ImageAspectFlagBits::PLANE_0_BIT)
                {
                   *out_plane_extent_ptr = in_image_extent;
                }
                else
                {
                   *out_plane_extent_ptr = {in_image_extent.width  / 2,
                                            in_image_extent.height / 2,
                                            in_image_extent.depth};
                }

                break;
            }

            case Anvil::Format::G8_B8_R8_3PLANE_422_UNORM:
            case Anvil::Format::G8_B8R8_2PLANE_422_UNORM:
            case Anvil::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
            case Anvil::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
            case Anvil::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
            case Anvil::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
            case Anvil::Format::G16_B16_R16_3PLANE_422_UNORM:
            case Anvil::Format::G16_B16R16_2PLANE_422_UNORM:
            {
                anvil_assert((in_image_extent.width % 2) == 0);

                if (in_aspect == Anvil::ImageAspectFlagBits::PLANE_0_BIT)
                {
                   *out_plane_extent_ptr = in_image_extent;
                }
                else
                {
                   *out_plane_extent_ptr = {in_image_extent.width / 2,
                                            in_image_extent.height,
                                            in_image_extent.depth};
                }

                break;
            }

            case Anvil::Format::G8_B8_R8_3PLANE_444_UNORM:
            case Anvil::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
            case Anvil::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
            case Anvil::Format::G16_B16_R16_3PLANE_444_UNORM:
            {
               *out_plane_extent_ptr = in_image_extent;

                break;
            }

            default:
            {
                anvil_assert_fail();
            }
        }
    }
}

/** Please see header for specification */
uint32_t Anvil::Formats::get_yuv_format_plane_index(Anvil::Format              in_format,
                                                    Anvil::ImageAspectFlagBits in_aspect)
{
    const auto n_planes  = Anvil::Formats::get_format_n_planes(in_format);
    uint32_t   plane_idx = 0;

    ANVIL_REDUNDANT_VARIABLE_CONST(n_planes);

    anvil_assert(0 < n_planes &&
                     n_planes <= 3);

    if (!Anvil::Formats::is_format_multiplanar(in_format) )
    {
        anvil_assert(n_planes  == 1);
        anvil_assert(in_aspect == Anvil::ImageAspectFlagBits::COLOR_BIT);

        plane_idx = 0;
    }
    else
    {
        anvil_assert(in_aspect == Anvil::ImageAspectFlagBits::PLANE_0_BIT ||
                     in_aspect == Anvil::ImageAspectFlagBits::PLANE_1_BIT ||
                     in_aspect == Anvil::ImageAspectFlagBits::PLANE_2_BIT);

        switch (in_aspect)
        {
            case Anvil::ImageAspectFlagBits::PLANE_0_BIT:
            {
                anvil_assert(n_planes >= 1);

                plane_idx = 0;
                break;
            }

            case Anvil::ImageAspectFlagBits::PLANE_1_BIT:
            {
                anvil_assert(n_planes >= 2);

                plane_idx = 1;
                break;
            }

            case Anvil::ImageAspectFlagBits::PLANE_2_BIT:
            {
                anvil_assert(n_planes == 3);

                plane_idx = 2;
                break;
            }

            default:
            {
                anvil_assert_fail();
            }
        }
    }

    return plane_idx;
}

/** Please see header for specification */
bool Anvil::Formats::has_depth_aspect(Anvil::Format in_format)
{
    if (!Anvil::Formats::is_format_yuv_khr(in_format) )
    {
        return (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::D)  ||
               (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::DS) ||
               (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::XD);
    }

    return false;
}

/** Please see header for specification */
bool Anvil::Formats::has_stencil_aspect(Anvil::Format in_format)
{
    if (!Anvil::Formats::is_format_yuv_khr(in_format) )
    {
        return (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::S)  ||
               (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::DS);
    }

    return false;
}

/** Please see header for specification */
bool Anvil::Formats::is_format_compressed(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE || Anvil::Formats::is_format_yuv_khr(in_format));

    if (Anvil::Formats::is_format_yuv_khr(in_format) )
    {
        return (g_yuv_formats.at(in_format).subresources[0].component_layout == Anvil::ComponentLayout::GBGR)     ||
               (g_yuv_formats.at(in_format).subresources[0].component_layout == Anvil::ComponentLayout::BGRG)     ||
               (g_yuv_formats.at(in_format).subresources[0].component_layout == Anvil::ComponentLayout::GXBXGXRX) ||
               (g_yuv_formats.at(in_format).subresources[0].component_layout == Anvil::ComponentLayout::BXGXRXGX);
    }
    else
    {
        return (g_formats[static_cast<uint32_t>(in_format)].component_bits[0] == g_formats[static_cast<uint32_t>(in_format)].component_bits[1]) &&
               (g_formats[static_cast<uint32_t>(in_format)].component_bits[1] == g_formats[static_cast<uint32_t>(in_format)].component_bits[2]) &&
               (g_formats[static_cast<uint32_t>(in_format)].component_bits[2] == g_formats[static_cast<uint32_t>(in_format)].component_bits[3]) &&
               (g_formats[static_cast<uint32_t>(in_format)].component_bits[0] == 0);
    }
}

/** Please see header for specification */
bool Anvil::Formats::is_format_multiplanar(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE || Anvil::Formats::is_format_yuv_khr(in_format) );

    if (Anvil::Formats::is_format_yuv_khr(in_format) )
    {
        return g_yuv_formats.at(in_format).is_multiplanar;
    }

    return false;
}

/** Please see header for specification */
bool Anvil::Formats::is_format_yuv_khr(Anvil::Format in_format)
{
    return (in_format == Anvil::Format::G8B8G8R8_422_UNORM                         ||
            in_format == Anvil::Format::B8G8R8G8_422_UNORM                         ||
            in_format == Anvil::Format::G8_B8_R8_3PLANE_420_UNORM                  ||
            in_format == Anvil::Format::G8_B8R8_2PLANE_420_UNORM                   ||
            in_format == Anvil::Format::G8_B8_R8_3PLANE_422_UNORM                  ||
            in_format == Anvil::Format::G8_B8R8_2PLANE_422_UNORM                   ||
            in_format == Anvil::Format::G8_B8_R8_3PLANE_444_UNORM                  ||
            in_format == Anvil::Format::R10X6_UNORM_PACK16                         ||
            in_format == Anvil::Format::R10X6G10X6_UNORM_2PACK16                   ||
            in_format == Anvil::Format::R10X6G10X6B10X6A10X6_UNORM_4PACK16         ||
            in_format == Anvil::Format::G10X6B10X6G10X6R10X6_422_UNORM_4PACK16     ||
            in_format == Anvil::Format::B10X6G10X6R10X6G10X6_422_UNORM_4PACK16     ||
            in_format == Anvil::Format::G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 ||
            in_format == Anvil::Format::G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16  ||
            in_format == Anvil::Format::G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 ||
            in_format == Anvil::Format::G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16  ||
            in_format == Anvil::Format::G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 ||
            in_format == Anvil::Format::R12X4_UNORM_PACK16                         ||
            in_format == Anvil::Format::R12X4G12X4_UNORM_2PACK16                   ||
            in_format == Anvil::Format::R12X4G12X4B12X4A12X4_UNORM_4PACK16         ||
            in_format == Anvil::Format::G12X4B12X4G12X4R12X4_422_UNORM_4PACK16     ||
            in_format == Anvil::Format::B12X4G12X4R12X4G12X4_422_UNORM_4PACK16     ||
            in_format == Anvil::Format::G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 ||
            in_format == Anvil::Format::G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16  ||
            in_format == Anvil::Format::G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 ||
            in_format == Anvil::Format::G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16  ||
            in_format == Anvil::Format::G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 ||
            in_format == Anvil::Format::G16B16G16R16_422_UNORM                     ||
            in_format == Anvil::Format::B16G16R16G16_422_UNORM                     ||
            in_format == Anvil::Format::G16_B16_R16_3PLANE_420_UNORM               ||
            in_format == Anvil::Format::G16_B16R16_2PLANE_420_UNORM                ||
            in_format == Anvil::Format::G16_B16_R16_3PLANE_422_UNORM               ||
            in_format == Anvil::Format::G16_B16R16_2PLANE_422_UNORM                ||
            in_format == Anvil::Format::G16_B16_R16_3PLANE_444_UNORM);
}

/** Please see header for specification */
bool Anvil::Formats::is_format_packed(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE || Anvil::Formats::is_format_yuv_khr(in_format) );

    if (Anvil::Formats::is_format_yuv_khr(in_format) )
    {
        return g_yuv_formats.at(in_format).is_packed;
    }
    else
    {
        return g_formats[static_cast<uint32_t>(in_format)].is_packed;
    }
}