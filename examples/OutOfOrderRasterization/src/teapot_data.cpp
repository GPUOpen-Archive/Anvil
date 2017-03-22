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

/* Teapot data comes from public source codepublished under ftp://ftp.funet.fi/pub/sci/graphics/packages/objects/teaset.tar.Z
 * (teapot dataset by Newell) */

#include "teapot_data.h"
#include <assert.h>
#include <stdint.h>

#define N_PATCH_VERTICES (16)


// Teapot patch definitions from "The Origins of the Teapot"
// Frank Crow, Xerox PARC (from conversations with Martin Newell and Jim Blinn)
// IEEE Computer Graphics and Applications
// Volume 7 Issue 1, January 1987
// Pages 8 - 19
const uint32_t TeapotData::m_patch_index_data[] =
{
    1,   2,   3,   4,
    5,   6,   7,   8,
    9,   10,  11,  12,
    13,  14,  15,  16,

    4,   17,  18,  19,
    8,   20,  21,  22,
    12,  23,  24,  25,
    16,  26,  27,  28,

    19,  29,  30,  31,
    22,  32,  33,  34,
    25,  35,  36,  37,
    28,  38,  39,  40,

    31,  41,  42,  1,
    34,  43,  44,  5,
    37,  45,  46,  9,
    40,  47,  48,  13,

    13,  14,  15,  16,
    49,  50,  51,  52,
    53,  54,  55,  56,
    57,  58,  59,  60,

    16,  26,  27,  28,
    52,  61,  62,  63,
    56,  64,  65,  66,
    60,  67,  68,  69,

    28,  38,  39,  40,
    63,  70,  71,  72,
    66,  73,  74,  75,
    69,  76,  77,  78,

    40,  47,  48,  13,
    72,  79,  80,  49,
    75,  81,  82,  53,
    78,  83,  84,  57,

    57,  58,  59,  60,
    85,  86,  87,  88,
    89,  90,  91,  92,
    93,  94,  95,  96,

    60,  67,  68,  69,
    88,  97,  98,  99,
    92,  100, 101, 102,
    96,  103, 104, 105,

    69,  76,  77,  78,
    99,  106, 107, 108,
    102, 109, 110, 111,
    105, 112, 113, 114,

    78,  83,  84,  57,
    108, 115, 116, 85,
    111, 117, 118, 89,
    114, 119, 120, 93,

    121, 122, 123, 124,
    125, 126, 127, 128,
    129, 130, 131, 132,
    133, 134, 135, 136,

    124, 137, 138, 121,
    128, 139, 140, 125,
    132, 141, 142, 129,
    136, 143, 144, 133,

    133, 134, 135, 136,
    145, 146, 147, 148,
    149, 150, 151, 152,
    69,  153, 154, 155,

    136, 143, 144, 133,
    148, 156, 157, 145,
    152, 158, 159, 149,
    155, 160, 161, 69,

    162, 163, 164, 165,
    166, 167, 168, 169,
    170, 171, 172, 173,
    174, 175, 176, 177,

    165, 178, 179, 162,
    169, 180, 181, 166,
    173, 182, 183, 170,
    177, 184, 185, 174,

    174, 175, 176, 177,
    186, 187, 188, 189,
    190, 191, 192, 193,
    194, 195, 196, 197,

    177, 184, 185, 174,
    189, 198, 199, 186,
    193, 200, 201, 190,
    197, 202, 203, 194,

    204, 204, 204, 204,
    207, 208, 209, 210,
    211, 211, 211, 211,
    212, 213, 214, 215,

    204, 204, 204, 204,
    210, 217, 218, 219,
    211, 211, 211, 211,
    215, 220, 221, 222,

    204, 204, 204, 204,
    219, 224, 225, 226,
    211, 211, 211, 211,
    222, 227, 228, 229,

    204, 204, 204, 204,
    226, 230, 231, 207,
    211, 211, 211, 211,
    229, 232, 233, 212,

    212, 213, 214, 215,
    234, 235, 236, 237,
    238, 239, 240, 241,
    242, 243, 244, 245,

    215, 220, 221, 222,
    237, 246, 247, 248,
    241, 249, 250, 251,
    245, 252, 253, 254,

    222, 227, 228, 229,
    248, 255, 256, 257,
    251, 258, 259, 260,
    254, 261, 262, 263,

    229, 232, 233, 212,
    257, 264, 265, 234,
    260, 266, 267, 238,
    263, 268, 269, 242,

    270, 270, 270, 270,
    279, 280, 281, 282,
    275, 276, 277, 278,
    271, 272, 273, 274,

    270, 270, 270, 270,
    282, 289, 290, 291,
    278, 286, 287, 288,
    274, 283, 284, 285,

    270, 270, 270, 270,
    291, 298, 299, 300,
    288, 295, 296, 297,
    285, 292, 293, 294,

    270, 270, 270, 270,
    300, 305, 306, 279,
    297, 303, 304, 275,
    294, 301, 302, 271,
};

// Teapot vertices from "The Origins of the Teapot"
// Frank Crow, Xerox PARC (from conversations with Martin Newell and Jim Blinn)
// IEEE Computer Graphics and Applications
// Volume 7 Issue 1, January 1987
// Pages 8 - 19
const float TeapotData::m_patch_vertex_data[] =
{
    1.4f,      0.0f,    2.4f,
    1.4f,     -0.784f,  2.4f,
    0.784f,   -1.4f,    2.4f,
    0.0f,     -1.4f,    2.4f,
    1.3375f,   0.0f,    2.53125f,
    1.3375f,  -0.749f,  2.53125f,
    0.749f,   -1.3375f, 2.53125f,
    0.0f,     -1.3375f, 2.53125f,
    1.4375f,   0.0f,    2.53125f,
    1.4375f,  -0.805f,  2.53125f,
    0.805f,   -1.4375f, 2.53125f,
    0.0f,     -1.4375f, 2.53125f,
    1.5f,      0.0f,    2.4f,
    1.5f,     -0.84f,   2.4f,
    0.84f,    -1.5f,    2.4f,
    0.0f,     -1.5f,    2.4f,
   -0.784f,   -1.4f,    2.4f,
   -1.4f,     -0.784f,  2.4f,
   -1.4f,      0.0f,    2.4f,
   -0.749f,   -1.3375f, 2.53125f,
   -1.3375f,  -0.749f,  2.53125f,
   -1.3375f,   0.0f,    2.53125f,
   -0.805f,   -1.4375f, 2.53125f,
   -1.4375f,  -0.805f,  2.53125f,
   -1.4375f,   0.0f,    2.53125f,
   -0.84f,    -1.5f,    2.4f,
   -1.5f,     -0.84f,   2.4f,
   -1.5f,      0.0f,    2.4f,
   -1.4f,      0.784f,  2.4f,
   -0.784f,    1.4f,    2.4f,
    0.0f,      1.4f,    2.4f,
   -1.3375f,   0.749f,  2.53125f,
   -0.749f,    1.3375f, 2.53125f,
    0.0f,      1.3375f, 2.53125f,
   -1.4375f,   0.805f,  2.53125f,
   -0.805f,    1.4375f, 2.53125f,
    0.0f,      1.4375f, 2.53125f,
   -1.5f,      0.84f,   2.4f,
   -0.84f,     1.5f,    2.4f,
    0.0f,      1.5f,    2.4f,
    0.784f,    1.4f,    2.4f,
    1.4f,      0.784f,  2.4f,
    0.749f,    1.3375f, 2.53125f,
    1.3375f,   0.749f,  2.53125f,
    0.805f,    1.4375f, 2.53125f,
    1.4375f,   0.805f,  2.53125f,
    0.84f,     1.5f,    2.4f,
    1.5f,      0.84f,   2.4f,
    1.75f,     0.0f,    1.875f,
    1.75f,    -0.98f,   1.875f,
    0.98f,    -1.75f,   1.875f,
    0.0f,     -1.75f,   1.875f,
    2.0f,      0.0f,    1.35f,
    2.0f,     -1.12f,   1.35f,
    1.12f,    -2.0f,    1.35f,
    0.0f,     -2.0f,    1.35f,
    2.0f,      0.0f,    0.9f,
    2.0f,     -1.12f,   0.9f,
    1.12f,    -2.0f,    0.9f,
    0.0f,     -2.0f,    0.9f,
   -0.98f,    -1.75f,   1.875f,
   -1.75f,    -0.98f,   1.875f,
   -1.75f,     0.0f,    1.875f,
   -1.12f,    -2.0f,    1.35f,
   -2.0f,     -1.12f,   1.35f,
   -2.0f,      0.0f,    1.35f,
   -1.12f,    -2.0f,    0.9f,
   -2.0f,     -1.12f,   0.9f,
   -2.0f,      0.0f,    0.9f,
   -1.75f,     0.98f,   1.875f,
   -0.98f,     1.75f,   1.875f,
    0.0f,      1.75f,   1.875f,
   -2.0f,      1.12f,   1.35f,
   -1.12f,     2.0f,    1.35f,
    0.0f,      2.0f,    1.35f,
   -2.0f,      1.12f,   0.9f,
   -1.12f,     2.0f,    0.9f,
    0.0f,      2.0f,    0.9f,
    0.98f,     1.75f,   1.875f,
    1.75f,     0.98f,   1.875f,
    1.12f,     2.0f,    1.35f,
    2.0f,      1.12f,   1.35f,
    1.12f,     2.0f,    0.9f,
    2.0f,      1.12f,   0.9f,
    2.0f,      0.0f,    0.45f,
    2.0f,     -1.12f,   0.45f,
    1.12f,    -2.0f,    0.45f,
    0.0f,     -2.0f,    0.45f,
    1.5f,      0.0f,    0.225f,
    1.5f,     -0.84f,   0.225f,
    0.84f,    -1.5f,    0.225f,
    0.0f,     -1.5f,    0.225f,
    1.5f,      0.0f,    0.15f,
    1.5f,     -0.84f,   0.15f,
    0.84f,    -1.5f,    0.15f,
    0.0f,     -1.5f,    0.15f,
   -1.12f,    -2.0f,    0.45f,
   -2.0f,     -1.12f,   0.45f,
   -2.0f,      0.0f,    0.45f,
   -0.84f,    -1.5f,    0.225f,
   -1.5f,     -0.84f,   0.225f,
   -1.5f,      0.0f,    0.225f,
   -0.84f,    -1.5f,    0.15f,
   -1.5f,     -0.84f,   0.15f,
   -1.5f,      0.0f,    0.15f,
   -2.0f,      1.12f,   0.45f,
   -1.12f,     2.0f,    0.45f,
    0.0f,      2.0f,    0.45f,
   -1.5f,      0.84f,   0.225f,
   -0.84f,     1.5f,    0.225f,
    0.0f,      1.5f,    0.225f,
   -1.5f,      0.84f,   0.15f,
   -0.84f,     1.5f,    0.15f,
    0.0f,      1.5f,    0.15f,
    1.12f,     2.0f,    0.45f,
    2.0f,      1.12f,   0.45f,
    0.84f,     1.5f,    0.225f,
    1.5f,      0.84f,   0.225f,
    0.84f,     1.5f,    0.15f,
    1.5f,      0.84f,   0.15f,
   -1.6f,      0.0f,    2.025f,
   -1.6f,     -0.3f,    2.025f,
   -1.5f,     -0.3f,    2.25f,
   -1.5f,      0.0f,    2.25f,
   -2.3f,      0.0f,    2.025f,
   -2.3f,     -0.3f,    2.025f,
   -2.5f,     -0.3f,    2.25f,
   -2.5f,      0.0f,    2.25f,
   -2.7f,      0.0f,    2.025f,
   -2.7f,     -0.3f,    2.025f,
   -3.0f,     -0.3f,    2.25f,
   -3.0f,      0.0f,    2.25f,
   -2.7f,      0.0f,    1.8f,
   -2.7f,     -0.3f,    1.8f,
   -3.0f,     -0.3f,    1.8f,
   -3.0f,      0.0f,    1.8f,
   -1.5f,      0.3f,    2.25f,
   -1.6f,      0.3f,    2.025f,
   -2.5f,      0.3f,    2.25f,
   -2.3f,      0.3f,    2.025f,
   -3.0f,      0.3f,    2.25f,
   -2.7f,      0.3f,    2.025f,
   -3.0f,      0.3f,    1.8f,
   -2.7f,      0.3f,    1.8f,
   -2.7f,      0.0f,    1.575f,
   -2.7f,     -0.3f,    1.575f,
   -3.0f,     -0.3f,    1.35f,
   -3.0f,      0.0f,    1.35f,
   -2.5f,      0.0f,    1.125f,
   -2.5f,     -0.3f,    1.125f,
   -2.65f,    -0.3f,    0.9375f,
   -2.65f,     0.0f,    0.9375f,
   -2.0f,     -0.3f,    0.9f,
   -1.9f,     -0.3f,    0.6f,
   -1.9f,      0.0f,    0.6f,
   -3.0f,      0.3f,    1.35f,
   -2.7f,      0.3f,    1.575f,
   -2.65f,     0.3f,    0.9375f,
   -2.5f,      0.3f,    1.125f,
   -1.9f,      0.3f,    0.6f,
   -2.0f,      0.3f,    0.9f,
    1.7f,      0.0f,    1.425f,
    1.7f,     -0.66f,   1.425f,
    1.7f,     -0.66f,   0.6f,
    1.7f,      0.0f,    0.6f,
    2.6f,      0.0f,    1.425f,
    2.6f,     -0.66f,   1.425f,
    3.1f,     -0.66f,   0.825f,
    3.1f,      0.0f,    0.825f,
    2.3f,      0.0f,    2.1f,
    2.3f,     -0.25f,   2.1f,
    2.4f,     -0.25f,   2.025f,
    2.4f,      0.0f,    2.025f,
    2.7f,      0.0f,    2.4f,
    2.7f,     -0.25f,   2.4f,
    3.3f,     -0.25f,   2.4f,
    3.3f,      0.0f,    2.4f,
    1.7f,      0.66f,   0.6f,
    1.7f,      0.66f,   1.425f,
    3.1f,      0.66f,   0.825f,
    2.6f,      0.66f,   1.425f,
    2.4f,      0.25f,   2.025f,
    2.3f,      0.25f,   2.1f,
    3.3f,      0.25f,   2.4f,
    2.7f,      0.25f,   2.4f,
    2.8f,      0.0f,    2.475f,
    2.8f,     -0.25f,   2.475f,
    3.525f,   -0.25f,   2.49375f,
    3.525f,    0.0f,    2.49375f,
    2.9f,      0.0f,    2.475f,
    2.9f,     -0.15f,   2.475f,
    3.45f,    -0.15f,   2.5125f,
    3.45f,     0.0f,    2.5125f,
    2.8f,      0.0f,    2.4f,
    2.8f,     -0.15f,   2.4f,
    3.2f,     -0.15f,   2.4f,
    3.2f,      0.0f,    2.4f,
    3.525f,    0.25f,   2.49375f,
    2.8f,      0.25f,   2.475f,
    3.45f,     0.15f,   2.5125f,
    2.9f,      0.15f,   2.475f,
    3.2f,      0.15f,   2.4f,
    2.8f,      0.15f,   2.4f,
    0.0f,      0.0f,    3.15f,
    0.0f,     -0.002f,  3.15f,
    0.002f,    0.0f,    3.15f,
    0.8f,      0.0f,    3.15f,
    0.8f,     -0.45f,   3.15f,
    0.45f,    -0.8f,    3.15f,
    0.0f,     -0.8f,    3.15f,
    0.0f,      0.0f,    2.85f,
    0.2f,      0.0f,    2.7f,
    0.2f,     -0.112f,  2.7f,
    0.112f,   -0.2f,    2.7f,
    0.0f,     -0.2f,    2.7f,
   -0.002f,    0.0f,    3.15f,
   -0.45f,    -0.8f,    3.15f,
   -0.8f,     -0.45f,   3.15f,
   -0.8f,      0.0f,    3.15f,
   -0.112f,   -0.2f,    2.7f,
   -0.2f,     -0.112f,  2.7f,
   -0.2f,      0.0f,    2.7f,
    0.0f,      0.002f,  3.15f,
   -0.8f,      0.45f,   3.15f,
   -0.45f,     0.8f,    3.15f,
    0.0f,      0.8f,    3.15f,
   -0.2f,      0.112f,  2.7f,
   -0.112f,    0.2f,    2.7f,
    0.0f,      0.2f,    2.7f,
    0.45f,     0.8f,    3.15f,
    0.8f,      0.45f,   3.15f,
    0.112f,    0.2f,    2.7f,
    0.2f,      0.112f,  2.7f,
    0.4f,      0.0f,    2.55f,
    0.4f,     -0.224f,  2.55f,
    0.224f,   -0.4f,    2.55f,
    0.0f,     -0.4f,    2.55f,
    1.3f,      0.0f,    2.55f,
    1.3f,     -0.728f,  2.55f,
    0.728f,   -1.3f,    2.55f,
    0.0f,     -1.3f,    2.55f,
    1.3f,      0.0f,    2.4f,
    1.3f,     -0.728f,  2.4f,
    0.728f,   -1.3f,    2.4f,
    0.0f,     -1.3f,    2.4f,
   -0.224f,   -0.4f,    2.55f,
   -0.4f,     -0.224f,  2.55f,
   -0.4f,      0.0f,    2.55f,
   -0.728f,   -1.3f,    2.55f,
   -1.3f,     -0.728f,  2.55f,
   -1.3f,      0.0f,    2.55f,
   -0.728f,   -1.3f,    2.4f,
   -1.3f,     -0.728f,  2.4f,
   -1.3f,      0.0f,    2.4f,
   -0.4f,      0.224f,  2.55f,
   -0.224f,    0.4f,    2.55f,
    0.0f,      0.4f,    2.55f,
   -1.3f,      0.728f,  2.55f,
   -0.728f,    1.3f,    2.55f,
    0.0f,      1.3f,    2.55f,
   -1.3f,      0.728f,  2.4f,
   -0.728f,    1.3f,    2.4f,
    0.0f,      1.3f,    2.4f,
    0.224f,    0.4f,    2.55f,
    0.4f,      0.224f,  2.55f,
    0.728f,    1.3f,    2.55f,
    1.3f,      0.728f,  2.55f,
    0.728f,    1.3f,    2.4f,
    1.3f,      0.728f,  2.4f,
    0.0f,      0.0f,    0.0f,
    1.5f,      0.0f,    0.15f,
    1.5f,      0.84f,   0.15f,
    0.84f,     1.5f,    0.15f,
    0.0f,      1.5f,    0.15f,
    1.5f,      0.0f,    0.075f,
    1.5f,      0.84f,   0.075f,
    0.84f,     1.5f,    0.075f,
    0.0f,      1.5f,    0.075f,
    1.425f,    0.0f,    0.0f,
    1.425f,    0.798f,  0.0f,
    0.798f,    1.425f,  0.0f,
    0.0f,      1.425f,  0.0f,
   -0.84f,     1.5f,    0.15f,
   -1.5f,      0.84f,   0.15f,
   -1.5f,      0.0f,    0.15f,
   -0.84f,     1.5f,    0.075f,
   -1.5f,      0.84f,   0.075f,
   -1.5f,      0.0f,    0.075f,
   -0.798f,    1.425f,  0.0f,
   -1.425f,    0.798f,  0.0f,
   -1.425f,    0.0f,    0.0f,
   -1.5f,     -0.84f,   0.15f,
   -0.84f,    -1.5f,    0.15f,
    0.0f,     -1.5f,    0.15f,
   -1.5f,     -0.84f,   0.075f,
   -0.84f,    -1.5f,    0.075f,
    0.0f,     -1.5f,    0.075f,
   -1.425f,   -0.798f,  0.0f,
   -0.798f,   -1.425f,  0.0f,
    0.0f,     -1.425f,  0.0f,
    0.84f,    -1.5f,    0.15f,
    1.5f,     -0.84f,   0.15f,
    0.84f,    -1.5f,    0.075f,
    1.5f,     -0.84f,   0.075f,
    0.798f,   -1.425f,  0.0f,
    1.425f,   -0.798f,  0.0f,
};

Vertex operator*(Vertex in1, const float& in2)
{
    Vertex result;

    result.x = in1.x * in2;
    result.y = in1.y * in2;
    result.z = in1.z * in2;

    return result;
}

TeapotData::TeapotData(uint32_t u_granularity,
                       uint32_t v_granularity)
    :m_u_granularity   (u_granularity),
     m_v_granularity   (v_granularity)
{
    assert(m_u_granularity >= 2);
    assert(m_v_granularity >= 2);

    polygonize();
    normalize();
}

TeapotData::~TeapotData()
{
}

Vertex TeapotData::compute_bezier_curve(const Vertex* points4,
                                        const float   t) const
{
    Vertex result;

    assert(t >= 0.0f && t <= 1.0f);

    const float coeff0 = (1.0f - t) * (1.0f - t) * (1.0f - t);
    const float coeff1 = 3.0f       * t          * (1.0f - t) * (1.0f - t);
    const float coeff2 = 3.0f       * t          * t          * (1.0f - t);
    const float coeff3 = t          * t          * t;

    return points4[0] * coeff0 + points4[1] * coeff1 + points4[2] * coeff2 + points4[3] * coeff3;
}

Vertex TeapotData::compute_bezier_surface(const Vertex* points16,
                                          const float   u,
                                          const float   v)
{
    Vertex curve_points[4];
    Vertex result;

    assert(u >= 0.0f && u <= 1.0f);
    assert(v >= 0.0f && v <= 1.0f);

    for (uint32_t n_patch_row = 0;
                  n_patch_row < 4;
                ++n_patch_row)
    {
        const Vertex current_points[] =
        {
            points16[n_patch_row * 4 + 0],
            points16[n_patch_row * 4 + 1],
            points16[n_patch_row * 4 + 2],
            points16[n_patch_row * 4 + 3]
        };

        curve_points[n_patch_row] = compute_bezier_curve(current_points,
                                                         u);
    }

    return compute_bezier_curve(curve_points,
                                v);
}

Vertex TeapotData::get_patch_vertex(const uint32_t* index_data,
                                    uint32_t        n) const
{
    const uint32_t n_defined_patch_vertices = sizeof(m_patch_vertex_data) / sizeof(m_patch_vertex_data[0]);
    Vertex         result;

    assert(n_defined_patch_vertices > (index_data[n] - 1) * 3 + 2);

    result.x = m_patch_vertex_data[(index_data[n] - 1) * 3 + 0];
    result.y = m_patch_vertex_data[(index_data[n] - 1) * 3 + 1];
    result.z = m_patch_vertex_data[(index_data[n] - 1) * 3 + 2];

    return result;
}

void TeapotData::normalize()
{
    uint32_t component_index = 0;
    float    min_xyz[3];
    float    max_xyz[3];
    uint32_t n_vertices = static_cast<uint32_t>(m_vertex_data.size() / 3 /* xyz */);

    assert(n_vertices > 0);


    for (uint32_t n_component = 0;
                  n_component < 3;
                ++n_component)
    {
        max_xyz[n_component] = m_vertex_data[n_component];
        min_xyz[n_component] = m_vertex_data[n_component];
    }

    for (auto iterator  = m_vertex_data.cbegin() + 3;
              iterator != m_vertex_data.cend();
            ++iterator, ++component_index)
    {
        if (component_index >= 3)
        {
            component_index = 0;
        }

        if (max_xyz[component_index] < *iterator)
        {
            max_xyz[component_index] = *iterator;
        }

        if (min_xyz[component_index] > *iterator)
        {
            min_xyz[component_index] = *iterator;
        }
    }

    component_index = 0;

    for (uint32_t n_vertex = 0;
                  n_vertex < n_vertices;
                ++n_vertex)
    {
        for (uint32_t n_component = 0;
                      n_component < 3;
                    ++n_component)
        {
            const float current_value = m_vertex_data[n_vertex * 3 + n_component];

            m_vertex_data[n_vertex * 3 + n_component] = (current_value - min_xyz[n_component]) / (max_xyz[n_component] - min_xyz[n_component]) - 0.5f;
        }
    }
}

void TeapotData::polygonize()
{
    static_assert(((sizeof(m_patch_index_data)  / sizeof(m_patch_index_data [0])) % N_PATCH_VERTICES) == 0,
                  "Teapot data corruption detected");

    for (uint32_t n_patch = 0;
                  n_patch < sizeof(m_patch_index_data) / sizeof(m_patch_index_data[0]) / N_PATCH_VERTICES;
                ++n_patch)
    {
        const uint32_t* patch_index_data_ptr = m_patch_index_data + N_PATCH_VERTICES * n_patch;

        polygonize_patch(patch_index_data_ptr);
    }
}

void TeapotData::polygonize_patch(const uint32_t* patch_index_data_ptr)
{
    Vertex         patch_vertex_data[N_PATCH_VERTICES];
    const uint32_t start_index                         = static_cast<uint32_t>(m_vertex_data.size() / 3 /* xyz */);

    for (uint32_t n_patch_vertex = 0;
                  n_patch_vertex < N_PATCH_VERTICES;
                ++n_patch_vertex)
    {
        patch_vertex_data[n_patch_vertex] = get_patch_vertex(patch_index_data_ptr,
                                                             n_patch_vertex);
    }

    /* Vertex data: */
    for (uint32_t n_u = 0;
                  n_u < m_u_granularity;
                ++n_u)
    {
        for (uint32_t n_v = 0;
                      n_v < m_v_granularity;
                    ++n_v)
        {
            const float u = float(n_u) / float(m_u_granularity - 1);
            const float v = float(n_v) / float(m_v_granularity - 1);

            const Vertex result = compute_bezier_surface(patch_vertex_data,
                                                         u,
                                                         v);

            m_vertex_data.push_back(result.x);
            m_vertex_data.push_back(result.y);
            m_vertex_data.push_back(result.z);
        }
    }

    /* Index data:
     *
     * Consider:
     *
     * - u_granularity of 4
     * - v_granularity of 4:
     *
     * This gives us a vertex grid as below:
     *
     * u
     *
     * ^
     * 12-13-14-15
     * |G |H |I |
     * 8--9--10-11
     * |D |E |F |
     * 4--5--6--7
     * |A |B |C |
     * 0--1--2--3-> v
     *
     * For simplicity, we're going to expose a triangle list. For the grid above,
     * we'd need the following triangles, assuming clockwise winding:
     *
     * Square A: 0-4-1, 4-5-1
     * Square B: 1-5-2, 5-6-2
     * Square C: 2-6-3, 6-7-3
     *
     * Square D: 4-8 -5, 8 -9 -5
     * Square E: 5-9 -6, 9 -10-6
     * Square F: 6-10-7, 10-11-7
     *
     * Square G: 8 -12-9, 12-13-9
     * Square H: 9 -13-10,13-14-10
     * Square I: 10-14-11,14-15-11
     *
     * Code below constructs the grid by following the rules above.
     */
    for (uint32_t n_square_y = 0;
                  n_square_y < m_v_granularity - 1;
                ++n_square_y)
    {
        for (uint32_t n_square_x = 0;
                      n_square_x < m_u_granularity - 1;
                    ++n_square_x)
        {
            m_index_data.push_back(start_index +  n_square_y      * m_v_granularity + n_square_x);
            m_index_data.push_back(start_index + (n_square_y + 1) * m_v_granularity + n_square_x);
            m_index_data.push_back(start_index +  n_square_y      * m_v_granularity + n_square_x + 1);

            m_index_data.push_back(start_index + (n_square_y + 1) * m_v_granularity + n_square_x);
            m_index_data.push_back(start_index + (n_square_y + 1) * m_v_granularity + n_square_x + 1);
            m_index_data.push_back(start_index +  n_square_y      * m_v_granularity + n_square_x + 1);
        }
    }
}