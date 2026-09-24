//==========================================================================================================
//  Copyright (C) Lockheed Martin Corporation 2018
//  All rights reserved
//==========================================================================================================
#pragma once
#include <cmath>
namespace P3DMath
{
    // Interpolate between two values.  a is between 0 and 1
#define LERP(x0, x1, a) ((x0) + ((a) * ((x1) - (x0))))

    struct float2
    {
        float2() noexcept
            : x(0.0f), y(0.0f)
        {}

        float2(float _x, float _y)
            : x(_x), y(_y)
        {}

        inline float2 operator - (const float2& rhs) const
        {
            return float2(x - rhs.x, y - rhs.y);
        }

        inline float2& operator -= (const float2& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }

        inline float2 operator + (const float2& rhs) const
        {
            return float2(x + rhs.x, y + rhs.y);
        }

        inline float2& operator += (const float2& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        inline float2 operator * (const float2& rhs) const
        {
            return float2(x * rhs.x, y * rhs.y);
        }

        inline float2 operator * (const float& rhs) const
        {
            return float2(x * rhs, y * rhs);
        }

        inline float2& operator *= (const float2& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            return *this;
        }

        inline float2& operator *= (const float& rhs)
        {
            x *= rhs;
            y *= rhs;
            return *this;
        }

        inline float2 operator / (const float2& rhs) const
        {
            return float2(x / rhs.x, y / rhs.y);
        }

        inline float2 operator / (const float& rhs) const
        {
            float invScalar = 1.0f / rhs;
            return float2(x * invScalar, y * invScalar);
        }

        inline float2& operator /= (const float2& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            return *this;
        }

        inline float2& operator /= (const float& rhs)
        {
            float invScalar = 1.0f / rhs;
            x *= invScalar;
            y *= invScalar;
            return *this;
        }

        inline bool operator == (const float2& rhs)
        {
            return (x == rhs.x && y == rhs.y);
        }

        float x, y;
    };

    struct uint2
    {
        uint2() noexcept
            : x(0), y(0)
        {}

        uint2(unsigned int _x, unsigned int _y)
            : x(_x), y(_y)
        {}

        inline uint2 operator - (const uint2& rhs) const
        {
            return uint2(x - rhs.x, y - rhs.y);
        }

        inline uint2& operator -= (const uint2& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }

        inline uint2 operator + (const uint2& rhs) const
        {
            return uint2(x + rhs.x, y + rhs.y);
        }

        inline uint2& operator += (const uint2& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }

        inline uint2 operator * (const uint2& rhs) const
        {
            return uint2(x * rhs.x, y * rhs.y);
        }

        inline uint2 operator * (const unsigned int& rhs) const
        {
            return uint2(x * rhs, y * rhs);
        }

        inline uint2& operator *= (const uint2& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            return *this;
        }

        inline uint2& operator *= (const unsigned int& rhs)
        {
            x *= rhs;
            y *= rhs;
            return *this;
        }

        inline uint2 operator / (const uint2& rhs) const
        {
            return uint2(x / rhs.x, y / rhs.y);
        }

        inline uint2 operator / (const unsigned int& rhs) const
        {
            return uint2(x / rhs, y / rhs);
        }

        inline uint2& operator /= (const uint2& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            return *this;
        }

        inline uint2& operator /= (const unsigned& rhs)
        {
            x /= rhs;
            y /= rhs;
            return *this;
        }

        inline bool operator == (const uint2& rhs)
        {
            return (x == rhs.x && y == rhs.y);
        }

        unsigned int x, y;
    };

    struct float3
    {
        float3() noexcept
            : x(0.0f), y(0.0f), z(0.0f)
        {}

        float3(float _x, float _y, float _z)
            : x(_x), y(_y), z(_z)
        {}

        inline float3 operator - (const float3& rhs) const
        {
            return float3(x - rhs.x, y - rhs.y, z - rhs.z);
        }

        inline float3& operator -= (const float3& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        inline float3 operator + (const float3& rhs) const
        {
            return float3(x + rhs.x, y + rhs.y, z + rhs.z);
        }

        inline float3& operator += (const float3& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        inline float3 operator * (const float3& rhs) const
        {
            return float3(x * rhs.x, y * rhs.y, z * rhs.z);
        }

        inline float3 operator * (const float& rhs) const
        {
            return float3(x * rhs, y * rhs, z * rhs);
        }

        inline float3& operator *= (const float3& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }

        inline float3& operator *= (const float& rhs)
        {
            x *= rhs;
            y *= rhs;
            z *= rhs;
            return *this;
        }

        inline float3 operator / (const float3& rhs) const
        {
            return float3(x / rhs.x, y / rhs.y, z / rhs.z);
        }

        inline float3 operator / (const float& rhs) const
        {
            float invScalar = 1.0f / rhs;
            return float3(x * invScalar, y * invScalar, z * invScalar);
        }

        inline float3& operator /= (const float3& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            return *this;
        }

        inline float3& operator /= (const float& rhs)
        {
            float invScalar = 1.0f / rhs;
            x *= invScalar;
            y *= invScalar;
            z *= invScalar;
            return *this;
        }

        inline bool operator == (const float3& rhs)
        {
            return (x == rhs.x && y == rhs.y && z == rhs.z);
        }

        float x, y, z;
    };

    struct uint3
    {
        uint3() noexcept
            : x(0), y(0), z(0)
        {}

        uint3(unsigned int _x, unsigned int _y, unsigned int _z)
            : x(_x), y(_y), z(_z)
        {}

        inline uint3 operator - (const uint3& rhs) const
        {
            return uint3(x - rhs.x, y - rhs.y, z - rhs.z);
        }

        inline uint3& operator -= (const uint3& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            return *this;
        }

        inline uint3 operator + (const uint3& rhs) const
        {
            return uint3(x + rhs.x, y + rhs.y, z + rhs.z);
        }

        inline uint3& operator += (const uint3& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            return *this;
        }

        inline uint3 operator * (const uint3& rhs) const
        {
            return uint3(x * rhs.x, y * rhs.y, z * rhs.z);
        }

        inline uint3 operator * (const unsigned int& rhs) const
        {
            return uint3(x * rhs, y * rhs, z * rhs);
        }

        inline uint3& operator *= (const uint3& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            return *this;
        }

        inline uint3& operator *= (const unsigned int& rhs)
        {
            x *= rhs;
            y *= rhs;
            z *= rhs;
            return *this;
        }

        inline uint3 operator / (const uint3& rhs) const
        {
            return uint3(x / rhs.x, y / rhs.y, z / rhs.z);
        }

        inline uint3 operator / (const unsigned int& rhs) const
        {
            return uint3(x / rhs, y / rhs, z / rhs);
        }

        inline uint3& operator /= (const uint3& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            return *this;
        }

        inline uint3& operator /= (const unsigned int& rhs)
        {
            x /= rhs;
            y /= rhs;
            z /= rhs;
            return *this;
        }

        inline bool operator == (const float3& rhs)
        {
            return (x == rhs.x && y == rhs.y && z == rhs.z);
        }

        unsigned int x, y, z;
    };

    struct float4
    {
        float4() noexcept
            : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
        {}

        float4(float _x, float _y, float _z, float _w)
            : x(_x), y(_y), z(_z), w(_w)
        {}

        inline float4 operator - (const float4& rhs) const
        {
            return float4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
        }

        inline float4& operator -= (const float4& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }

        inline float4 operator + (const float4& rhs) const
        {
            return float4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
        }

        inline float4& operator += (const float4& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }

        inline float4 operator * (const float4& rhs) const
        {
            return float4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
        }

        inline float4 operator * (const float& rhs) const
        {
            return float4(x * rhs, y * rhs, z * rhs, w * rhs);
        }

        inline float4& operator *= (const float4& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }

        inline float4& operator *= (const float& rhs)
        {
            x *= rhs;
            y *= rhs;
            z *= rhs;
            w *= rhs;
            return *this;
        }

        inline float4 operator / (const float4& rhs) const
        {
            return float4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
        }

        inline float4 operator / (const float& rhs) const
        {
            float invScalar = 1.0f / rhs;
            return float4(x * invScalar, y * invScalar, z * invScalar, w * invScalar);
        }

        inline float4& operator /= (const float4& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            w /= rhs.w;
            return *this;
        }

        inline float4& operator /= (const float& rhs)
        {
            float invScalar = 1.0f / rhs;
            x *= invScalar;
            y *= invScalar;
            z *= invScalar;
            w *= invScalar;
            return *this;
        }

        inline bool operator == (const float4& rhs)
        {
            return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
        }

        inline float4& operator = (const float rhs[4])
        {
            x = rhs[0];
            y = rhs[1];
            z = rhs[2];
            w = rhs[3];
            return *this;
        }

        float x, y, z, w;
    };

    struct uint4
    {
        uint4() noexcept
            : x(0), y(0), z(0), w(0)
        {}

        uint4(unsigned int _x, unsigned int _y, unsigned int _z, unsigned int _w)
            : x(_x), y(_y), z(_z), w(_w)
        {}

        inline uint4 operator - (const uint4& rhs) const
        {
            return uint4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
        }

        inline uint4& operator -= (const uint4& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }

        inline uint4 operator + (const uint4& rhs) const
        {
            return uint4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
        }

        inline uint4& operator += (const uint4& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }

        inline uint4 operator * (const uint4& rhs) const
        {
            return uint4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
        }

        inline uint4 operator * (const unsigned int& rhs) const
        {
            return uint4(x * rhs, y * rhs, z * rhs, w * rhs);
        }

        inline uint4& operator *= (const uint4& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }

        inline uint4& operator *= (const unsigned int& rhs)
        {
            x *= rhs;
            y *= rhs;
            z *= rhs;
            w *= rhs;
            return *this;
        }

        inline uint4 operator / (const uint4& rhs) const
        {
            return uint4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
        }

        inline uint4 operator / (const unsigned int& rhs) const
        {
            return uint4(x / rhs, y / rhs, z / rhs, w / rhs);
        }

        inline uint4& operator /= (const uint4& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            w /= rhs.w;
            return *this;
        }

        inline uint4& operator /= (const unsigned int& rhs)
        {
            x /= rhs;
            y /= rhs;
            z /= rhs;
            w /= rhs;
            return *this;
        }

        inline bool operator == (const uint4& rhs)
        {
            return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
        }

        unsigned int x, y, z, w;
    };

    inline float Length(const float2& vec)
    {
        return(sqrt(vec.x * vec.x + vec.y * vec.y));
    }

    inline float2 Normalize(const float2& vec)
    {
        float length = Length(vec);

        //Protect against length of zero.
        if (length == 0)
        {
            return float2();
        }
        else
        {
            return vec / length;
        }
    }

    inline float Dot(const float2& vec, const float2& vecRhs)
    {
        return (vec.x * vecRhs.x) + (vec.y * vecRhs.y);
    }

    inline float Length(const float3& vec)
    {
        return(sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z));
    }

    inline float3 Normalize(const float3& vec)
    {
        float length = Length(vec);

        //Protect against length of zero.
        if (length == 0)
        {
            return float3();
        }
        else
        {
            return vec / length;
        }
    }

    inline float Dot(const float3& vec, const float3& vecRhs)
    {
        return (vec.x * vecRhs.x) + (vec.y * vecRhs.y) + (vec.z * vecRhs.z);
    }

    inline float Length(const float4& vec)
    {
        return(sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w));
    }

    inline float4 Normalize(const float4& vec)
    {
        float length = Length(vec);

        //Protect against length of zero.
        if (length == 0)
        {
            return float4();
        }
        else
        {
            return vec / length;
        }
    }

    inline float Dot(const float4& vec, const float4& vecRhs)
    {
        return (vec.x * vecRhs.x) + (vec.y * vecRhs.y) + (vec.z * vecRhs.z) + (vec.w + vecRhs.w);
    }

    struct float4x4;

    struct float3x4
    {
        float3x4() noexcept : x(1.0f, 0.0f, 0.0f, 0.0f),
            y(0.0f, 1.0f, 0.0f, 0.0f),
            z(0.0f, 0.0f, 1.0f, 0.0f)
        {

        }

        float3x4(float4x4& mat3x4);

        union
        {
            struct
            {
                float4 x;
                float4 y;
                float4 z;
            };
            float m[3][4];
        };
    };

    struct float4x4
    {
        float4x4() noexcept : x(1.0f, 0.0f, 0.0f, 0.0f),
            y(0.0f, 1.0f, 0.0f, 0.0f),
            z(0.0f, 0.0f, 1.0f, 0.0f),
            w(0.0f, 0.0f, 0.0f, 1.0f)
        {
        }

        float4x4(float3x4& mat3x4) : x(mat3x4.x), y(mat3x4.y), z(mat3x4.z), w(0.0f, 0.0f, 0.0f, 1.0f)
        {
        }

        union
        {
            struct
            {
                float4 x;
                float4 y;
                float4 z;
                float4 w;
            };
            float m[4][4];
        };
    };

    inline float3x4::float3x4(float4x4& mat3x4) : x(mat3x4.x), y(mat3x4.y), z(mat3x4.z)
    {

    }

    struct SideAngles
    {
        float left;
        float right;
        float top;
        float bottom;
        SideAngles(float l, float r, float t, float b) : left(l), right(r), top(b), bottom(t) {}
        SideAngles() : left(0), right(0), top(0), bottom(0) {}
        float GetHFOV() { return right - left; }
        float GetVFOV() { return top - bottom; }
        float GetCenterX() { return 0.5f*(right + left); }
        float GetCenterY() { return 0.5f*(top + bottom); }
        bool IsUndefined() { return left == 0.0f && right == 0.0f && top == 0.0f && bottom == 0.0f; }
    };
}
