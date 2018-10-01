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

static const struct
{
    Anvil::Format format;
    uint32_t      red_component_start_bit_index;
    uint32_t      red_component_last_bit_index;
    uint32_t      green_component_start_bit_index;
    uint32_t      green_component_last_bit_index;
    uint32_t      blue_component_start_bit_index;
    uint32_t      blue_component_last_bit_index;
    uint32_t      alpha_component_start_bit_index;
    uint32_t      alpha_component_last_bit_index;
    uint32_t      shared_component_start_bit_index;
    uint32_t      shared_component_last_bit_index;
    uint32_t      depth_component_start_bit_index;
    uint32_t      depth_component_last_bit_index;
    uint32_t      stencil_component_start_bit_index;
    uint32_t      stencil_component_last_bit_index;
} g_format_bit_layout_info[] =
{
    /* format                                   | red start | red end   | green start | green end | blue start | blue end   | alpha start | alpha end | shared_start | shared_end | depth start | depth end | stencil start | stencil end */
    {Anvil::Format::UNKNOWN,                      UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R4G4_UNORM_PACK8,             4,          7,          0,            3,          UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R4G4B4A4_UNORM_PACK16,        12,         15,         8,            11,         4,           7,           0,            3,          UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B4G4R4A4_UNORM_PACK16,        4,          7,          8,            11,         12,          15,          0,            3,          UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R5G6B5_UNORM_PACK16,          11,         15,         5,            10,         0,           4,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B5G6R5_UNORM_PACK16,          0,          4,          5,            10,         11,          15,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R5G5B5A1_UNORM_PACK16,        11,         15,         6,            10,         1,           5,           0,            0,          UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B5G5R5A1_UNORM_PACK16,        1,          5,          6,            10,         11,          15,          0,            0,          UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A1R5G5B5_UNORM_PACK16,        10,         14,         5,            9,          0,           4,           15,           15,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8_UNORM,                     0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8_SNORM,                     0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8_USCALED,                   0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8_SSCALED,                   0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8_UINT,                      0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8_SINT,                      0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8_SRGB,                      0,          7,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8_UNORM,                   0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8_SNORM,                   0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8_USCALED,                 0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8_SSCALED,                 0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8_UINT,                    0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8_SINT,                    0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8_SRGB,                    0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8_UNORM,                 0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8_SNORM,                 0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8_USCALED,               0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8_SSCALED,               0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8_UINT,                  0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8_SINT,                  0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8_SRGB,                  0,          7,          8,            15,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8_UNORM,                 16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8_SNORM,                 16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8_USCALED,               16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8_SSCALED,               16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8_UINT,                  16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8_SINT,                  16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8_SRGB,                  16,         23,         8,            15,         0,           7,           UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8A8_UNORM,               0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8A8_SNORM,               0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8A8_USCALED,             0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8A8_SSCALED,             0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8A8_UINT,                0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8A8_SINT,                0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R8G8B8A8_SRGB,                0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8A8_UNORM,               16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8A8_SNORM,               16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8A8_USCALED,             16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8A8_SSCALED,             16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8A8_UINT,                16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8A8_SINT,                16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B8G8R8A8_SRGB,                16,         23,         8,            15,         0,           7,           24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A8B8G8R8_UNORM_PACK32,        0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A8B8G8R8_SNORM_PACK32,        0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A8B8G8R8_USCALED_PACK32,      0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A8B8G8R8_SSCALED_PACK32,      0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A8B8G8R8_UINT_PACK32,         0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A8B8G8R8_SINT_PACK32,         0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A8B8G8R8_SRGB_PACK32,         0,          7,          8,            15,         16,          23,          24,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2R10G10B10_UNORM_PACK32,     20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2R10G10B10_SNORM_PACK32,     20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2R10G10B10_USCALED_PACK32,   20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2R10G10B10_SSCALED_PACK32,   20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2R10G10B10_UINT_PACK32,      20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2R10G10B10_SINT_PACK32,      20,         29,         10,           19,         0,           9,           30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2B10G10R10_UNORM_PACK32,     0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2B10G10R10_SNORM_PACK32,     0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2B10G10R10_USCALED_PACK32,   0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2B10G10R10_SSCALED_PACK32,   0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2B10G10R10_UINT_PACK32,      0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::A2B10G10R10_SINT_PACK32,      0,          9,          10,           19,         20,          29,          30,           31,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16_UNORM,                    0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16_SNORM,                    0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16_USCALED,                  0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16_SSCALED,                  0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16_UINT,                     0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16_SINT,                     0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16_SFLOAT,                   0,          15,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16_UNORM,                 0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16_SNORM,                 0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16_USCALED,               0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16_SSCALED,               0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16_UINT,                  0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16_SINT,                  0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16_SFLOAT,                0,          15,         16,           31,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16_UNORM,              0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16_SNORM,              0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16_USCALED,            0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16_SSCALED,            0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16_UINT,               0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16_SINT,               0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16_SFLOAT,             0,          15,         16,           31,         32,          47,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16A16_UNORM,           0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16A16_SNORM,           0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16A16_USCALED,         0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16A16_SSCALED,         0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16A16_UINT,            0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16A16_SINT,            0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R16G16B16A16_SFLOAT,          0,          15,         16,           31,         32,          47,          48,           63,         UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32_UINT,                     0,          31,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32_SINT,                     0,          31,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32_SFLOAT,                   0,          31,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32_UINT,                  0,          31,         32,           63,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32_SINT,                  0,          31,         32,           63,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32_SFLOAT,                0,          31,         32,           63,         UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32B32_UINT,               0,          31,         32,           63,         64,          95,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32B32_SINT,               0,          31,         32,           63,         64,          95,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32B32_SFLOAT,             0,          31,         32,           63,         64,          95,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32B32A32_UINT,            0,          31,         32,           63,         64,          95,          96,           127,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32B32A32_SINT,            0,          31,         32,           63,         64,          95,          96,           127,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R32G32B32A32_SFLOAT,          0,          31,         32,           63,         64,          95,          96,           127,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64_UINT,                     0,          63,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64_SINT,                     0,          63,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64_SFLOAT,                   0,          63,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64_UINT,                  0,          63,         64,           127,        UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64_SINT,                  0,          63,         64,           127,        UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64_SFLOAT,                0,          63,         64,           127,        UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64B64_UINT,               0,          63,         64,           127,        128,         191,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64B64_SINT,               0,          63,         64,           127,        128,         191,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64B64_SFLOAT,             0,          63,         64,           127,        128,         191,         UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64B64A64_UINT,            0,          63,         64,           127,        128,         191,         192,          255,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64B64A64_SINT,            0,          63,         64,           127,        128,         191,         192,          255,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::R64G64B64A64_SFLOAT,          0,          63,         64,           127,        128,         191,         192,          255,        UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::B10G11R11_UFLOAT_PACK32,      0,          10,         11,           21,         22,          31,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::E5B9G9R9_UFLOAT_PACK32,       0,          8,          9,            17,         18,          26,          UINT32_MAX,   UINT32_MAX, 27,            31,          UINT32_MAX,   UINT32_MAX, UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::D16_UNORM,                    UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            15,         UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::X8_D24_UNORM_PACK32,          UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            23,         UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::D32_SFLOAT,                   UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            31,         UINT32_MAX,     UINT32_MAX},
    {Anvil::Format::S8_UINT,                      UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  UINT32_MAX,   UINT32_MAX, 0,              7},
    {Anvil::Format::D16_UNORM_S8_UINT,            UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            15,         16,             23},
    {Anvil::Format::D24_UNORM_S8_UINT,            UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            23,         24,             31},
    {Anvil::Format::D32_SFLOAT_S8_UINT,           UINT32_MAX, UINT32_MAX, UINT32_MAX,   UINT32_MAX, UINT32_MAX,  UINT32_MAX,  UINT32_MAX,   UINT32_MAX, UINT32_MAX,    UINT32_MAX,  0,            31,         32,             39},
};
static uint32_t g_layout_to_n_components[] =
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

    {
        /* This image can hold color and/or depth and/or stencil aspects only */
        const Anvil::ComponentLayout format_layout = Anvil::Formats::get_format_component_layout(in_format);

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
void Anvil::Formats::get_format_bit_layout(Anvil::Format in_format,
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
    anvil_assert(sizeof(g_format_bit_layout_info) / sizeof(g_format_bit_layout_info[0]) >  static_cast<uint32_t>(in_format) );
    anvil_assert(g_format_bit_layout_info[static_cast<uint32_t>(in_format)].format      == in_format);

    const auto& format_info = g_format_bit_layout_info[static_cast<uint32_t>(in_format)];

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

/** Please see header for specification */
Anvil::ComponentLayout Anvil::Formats::get_format_component_layout(Anvil::Format in_format)
{
    static_assert(sizeof(g_formats) / sizeof(g_formats[0]) == VK_FORMAT_RANGE_SIZE, "");
    anvil_assert(static_cast<uint32_t>(in_format)          <  VK_FORMAT_RANGE_SIZE);

    return g_formats[static_cast<uint32_t>(in_format)].component_layout;
}

/** Please see header for specification */
uint32_t Anvil::Formats::get_format_n_components(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE);

    return g_layout_to_n_components[static_cast<uint32_t>(g_formats[static_cast<uint32_t>(in_format)].component_layout)];
}

/** Please see header for specification */
void Anvil::Formats::get_format_n_component_bits(Anvil::Format in_format,
                                                 uint32_t*     out_channel0_bits_ptr,
                                                 uint32_t*     out_channel1_bits_ptr,
                                                 uint32_t*     out_channel2_bits_ptr,
                                                 uint32_t*     out_channel3_bits_ptr)
{
    const FormatInfo* format_props_ptr = nullptr;

    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE);

    format_props_ptr = g_formats + static_cast<uint32_t>(in_format);

    *out_channel0_bits_ptr = format_props_ptr->component_bits[0];
    *out_channel1_bits_ptr = format_props_ptr->component_bits[1];
    *out_channel2_bits_ptr = format_props_ptr->component_bits[2];
    *out_channel3_bits_ptr = format_props_ptr->component_bits[3];
}

/** Please see header for specification */
const char* Anvil::Formats::get_format_name(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE);

    return g_formats[static_cast<uint32_t>(in_format)].name;
}

/** Please see header for specification */
Anvil::FormatType Anvil::Formats::get_format_type(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE);

    return g_formats[static_cast<uint32_t>(in_format)].format_type;
}

/** Please see header for specification */
bool Anvil::Formats::has_depth_aspect(Anvil::Format in_format)
{
    return (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::D)  ||
           (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::DS) ||
           (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::XD);
}

/** Please see header for specification */
bool Anvil::Formats::has_stencil_aspect(Anvil::Format in_format)
{
    return (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::S)  ||
           (g_formats[static_cast<uint32_t>(in_format)].component_layout == Anvil::ComponentLayout::DS);
}

/** Please see header for specification */
bool Anvil::Formats::is_format_compressed(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE);

    return (g_formats[static_cast<uint32_t>(in_format)].component_bits[0] == g_formats[static_cast<uint32_t>(in_format)].component_bits[1]) &&
           (g_formats[static_cast<uint32_t>(in_format)].component_bits[1] == g_formats[static_cast<uint32_t>(in_format)].component_bits[2]) &&
           (g_formats[static_cast<uint32_t>(in_format)].component_bits[2] == g_formats[static_cast<uint32_t>(in_format)].component_bits[3]) &&
           (g_formats[static_cast<uint32_t>(in_format)].component_bits[0] == 0);
}

/** Please see header for specification */
bool Anvil::Formats::is_format_packed(Anvil::Format in_format)
{
    anvil_assert(static_cast<uint32_t>(in_format) < VK_FORMAT_RANGE_SIZE);

    return g_formats[static_cast<uint32_t>(in_format)].is_packed;
}