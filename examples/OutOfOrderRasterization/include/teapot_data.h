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
#include <limits>
#include <stdint.h>
#include <memory>
#include <vector>

typedef struct Vertex
{
    float x;
    float y;
    float z;

    Vertex& operator+(const Vertex& in)
    {
        this->x += in.x;
        this->y += in.y;
        this->z += in.z;

        return *this;
    }

    Vertex& operator*(const float& in)
    {
        x *= in;
        y *= in;
        z *= in;

        return *this;
    }

    Vertex& operator=(const Vertex& in)
    {
        this->x = in.x;
        this->y = in.y;
        this->z = in.z;

        return *this;
    }

    Vertex()
    {
        /* Use NaNs by default */
        x = std::numeric_limits<float>::quiet_NaN();
        y = std::numeric_limits<float>::quiet_NaN();
        z = std::numeric_limits<float>::quiet_NaN();
    }

    Vertex(const Vertex& in)
    {
        x = in.x;
        y = in.y;
        z = in.z;
    }
} Vertex;

class TeapotData
{
public:
    /* Public type definitions */

    /* Public functions */
     TeapotData(uint32_t u_granularity,
                uint32_t v_granularity);
    ~TeapotData();

    const uint32_t* get_index_data() const
    {
        return &m_index_data[0];
    }

    uint32_t get_index_data_size() const
    {
        return static_cast<uint32_t>(m_index_data.size() * sizeof(uint32_t) );
    }

    const float* get_vertex_data() const
    {
        return &m_vertex_data[0];
    }

    uint32_t get_vertex_data_size() const
    {
        return static_cast<uint32_t>(m_vertex_data.size() * sizeof(float) );
    }

private:
    /* Private type definitions */

    /* Private functions */
    TeapotData& operator=(const TeapotData&);
    TeapotData           (const TeapotData&);

    Vertex compute_bezier_curve  (const Vertex* points4,
                                  const float   t) const;
    Vertex compute_bezier_surface(const Vertex* points16,
                                  const float   u,
                                  const float   v);

    Vertex get_patch_vertex      (const uint32_t* index_data,
                                  uint32_t        n) const;

    void   normalize             ();
    void   polygonize            ();
    void   polygonize_patch      (const uint32_t* patch_index_data_ptr);

    /* Private variables */
    std::vector<uint32_t> m_index_data;
    std::vector<float>    m_vertex_data;

    const uint32_t m_u_granularity;
    const uint32_t m_v_granularity;

    static const uint32_t m_patch_index_data [];
    static const float    m_patch_vertex_data[];
};

