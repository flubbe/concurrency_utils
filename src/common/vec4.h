/**
 * concurrency_utils - concurrency utility library
 * 
 * implements a minimal set of vec4 operations used in the tests.
 * 
 * \author Felix Lubbe
 * \copyright Copyright (c) 2021
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#include <cmath>

struct vec4
{
    float x{0}, y{0}, z{0}, w{0};

    vec4() = default;

    vec4(float in_x, float in_y, float in_z, float in_w = 0.f)
    : x{in_x}
    , y{in_y}
    , z{in_z}
    , w{in_w}
    {
    }

    bool is_zero() const
    {
        return x == 0 && y == 0 && z == 0 && w == 0;
    }

    float length_squared() const
    {
        return dot_product(*this);
    }

    float length() const
    {
#ifdef __GNUC__
        return sqrtf(length_squared());
#else
        return std::sqrtf(length_squared());
#endif
    }

    float one_over_length() const
    {
        if(is_zero())
        {
            return 1.0f;
        }

        return 1.0f / length();
    }

    float dot_product(const vec4& v) const
    {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    void normalize()
    {
        /* OneOverLength is safe to call on zero vectors - no check needed. */
        *this = *this * one_over_length();
    }
    const vec4 normalized() const
    {
        return *this * one_over_length();
    }

    const vec4 scale(const float& S) const
    {
        return {x * S, y * S, z * S, w * S};
    }

    /* operations. */
    const vec4 operator+(const vec4& Other) const
    {
        return {x + Other.x, y + Other.y, z + Other.z, w + Other.w};
    }
    const vec4 operator-(const vec4& Other) const
    {
        return {x - Other.x, y - Other.y, z - Other.z, w - Other.w};
    }
    const vec4 operator*(float S) const
    {
        return scale(S);
    }
};

/*
 * helpers.
 */

/** dot product between two vectors. */
template<typename T>
float dot(const T a, const T b)
{
    return a.dot_product(b);
}
