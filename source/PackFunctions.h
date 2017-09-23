#pragma once

#include <stdint.h>
#include <math.h>

namespace Pack
{
    void RGBA32UintToRGBA8Uint( uint32_t count, const uint32_t* in, uint8_t* out )
    {
        for ( uint32_t i = 0; i < count; ++i )
        {
            out[ i * 4 + 0 ] = static_cast< uint8_t >( in[ i * 4 + 0 ] );
            out[ i * 4 + 1 ] = static_cast< uint8_t >( in[ i * 4 + 1 ] );
            out[ i * 4 + 2 ] = static_cast< uint8_t >( in[ i * 4 + 2 ] );
            out[ i * 4 + 3 ] = static_cast< uint8_t >( in[ i * 4 + 3 ] );
        }
    }

    void RGB32UintToRGB16Uint( uint32_t count, const uint32_t* in, uint16_t* out )
    {
        for ( uint32_t i = 0; i < count; ++i )
        {
            out[ i * 3 + 0 ] = static_cast< uint16_t >( in[ i * 3 + 0 ] );
            out[ i * 3 + 1 ] = static_cast< uint16_t >( in[ i * 3 + 1 ] );
            out[ i * 3 + 2 ] = static_cast< uint16_t >( in[ i * 3 + 2 ] );
        }
    }

    void RGBA32FloatToRGBA8Unorm( uint32_t count, const float* in, uint8_t* out )
    {
        for ( uint32_t i = 0; i < count; ++i )
        {
            float r = in[ i * 4 + 0 ];
            float g = in[ i * 4 + 1 ];
            float b = in[ i * 4 + 2 ];
            float a = in[ i * 4 + 3 ];

            // Convert from float scale to integer scale
            r = r * 255.f + 0.5f;
            g = g * 255.f + 0.5f;
            b = b * 255.f + 0.5f;
            a = a * 255.f + 0.5f;

            out[ i * 4 + 0 ] = static_cast< uint8_t >( r );
            out[ i * 4 + 1 ] = static_cast< uint8_t >( g );
            out[ i * 4 + 2 ] = static_cast< uint8_t >( b );
            out[ i * 4 + 3 ] = static_cast< uint8_t >( a );
        }
    }

    void RGB32FloatToRGBM8Unorm( uint32_t count, const float* in, uint8_t* out, float* out_q )
    {
        // Find the absolute maximum value of all floats to get the quantization scale
        float q = 1.f / 255.f;
        for ( uint32_t i = 0; i < count; ++i )
        {
            float r = in[ i * 3 + 0 ];
            float g = in[ i * 3 + 1 ];
            float b = in[ i * 3 + 2 ];

            q = fmaxf( q, fabsf( r ) );
            q = fmaxf( q, fabsf( g ) );
            q = fmaxf( q, fabsf( b ) );
        }

        for ( uint32_t i = 0; i < count; ++i )
        {
            float r = in[ i * 3 + 0 ];
            float g = in[ i * 3 + 1 ];
            float b = in[ i * 3 + 2 ];

            // Divide by the quantization scale, now within [-1, 1]
            r /= q;
            g /= q;
            b /= q;

            // Find the absolute maximum value of x, y and z to get the magnitude
            float m = fmaxf( fmaxf( 1.f / 255.f, fabsf( r ) ), fmaxf( fabsf( g ), fabsf( b ) ) );

            // Divide by the magnitude, still within [-1, 1]
            r /= m;
            g /= m;
            b /= m;

            // [-1, 1] -> [0, 1]
            r = r * 0.5f + 0.5f;
            g = g * 0.5f + 0.5f;
            b = b * 0.5f + 0.5f;

            // Convert from float scale to integer scale
            r = r * 255.f + 0.5f;
            g = g * 255.f + 0.5f;
            b = b * 255.f + 0.5f;
            m = m * 255.f + 0.5f;

            out[ i * 4 + 0 ] = static_cast< uint8_t >( r );
            out[ i * 4 + 1 ] = static_cast< uint8_t >( g );
            out[ i * 4 + 2 ] = static_cast< uint8_t >( b );
            out[ i * 4 + 3 ] = static_cast< uint8_t >( m );
        }

        // Store the quantization scale
        *out_q = q;
    }

    void RGB32FloatToRGBM16Unorm( uint32_t count, const float* in, uint16_t* out, float* out_q )
    {
        // Find the absolute maximum value of all floats to get the quantization scale
        float q = 1.f / 65535.f;
        for ( uint32_t i = 0; i < count; ++i )
        {
            float r = in[ i * 3 + 0 ];
            float g = in[ i * 3 + 1 ];
            float b = in[ i * 3 + 2 ];

            q = fmaxf( q, fabsf( r ) );
            q = fmaxf( q, fabsf( g ) );
            q = fmaxf( q, fabsf( b ) );
        }

        for ( uint32_t i = 0; i < count; ++i )
        {
            float r = in[ i * 3 + 0 ];
            float g = in[ i * 3 + 1 ];
            float b = in[ i * 3 + 2 ];

            // Divide by the quantization scale, now within [-1, 1]
            r /= q;
            g /= q;
            b /= q;

            // Find the absolute maximum value of x, y and z to get the magnitude
            float m = fmaxf( fmaxf( 1.f / 65535.f, fabsf( r ) ), fmaxf( fabsf( g ), fabsf( b ) ) );

            // Divide by the magnitude, still within [-1, 1]
            r /= m;
            g /= m;
            b /= m;

            // [-1, 1] -> [0, 1]
            r = r * 0.5f + 0.5f;
            g = g * 0.5f + 0.5f;
            b = b * 0.5f + 0.5f;

            // Convert from float scale to integer scale
            r = r * 65535.f + 0.5f;
            g = g * 65535.f + 0.5f;
            b = b * 65535.f + 0.5f;
            m = m * 65535.f + 0.5f;

            out[ i * 4 + 0 ] = static_cast< uint16_t >( r );
            out[ i * 4 + 1 ] = static_cast< uint16_t >( g );
            out[ i * 4 + 2 ] = static_cast< uint16_t >( b );
            out[ i * 4 + 3 ] = static_cast< uint16_t >( m );
        }

        // Store the quantization scale
        *out_q = q;
    }

    void TangentsToRGBA8Unorm( uint32_t count, const float* tangents, const float* bitangents, const float* normals, uint8_t* out )
    {
        for ( uint32_t i = 0; i < count; ++i )
        {
            float tx = tangents[ i * 3 + 0 ];
            float ty = tangents[ i * 3 + 1 ];
            float tz = tangents[ i * 3 + 2 ];

            float bx = bitangents[ i * 3 + 0 ];
            float by = bitangents[ i * 3 + 1 ];
            float bz = bitangents[ i * 3 + 2 ];

            float nx = normals[ i * 3 + 0 ];
            float ny = normals[ i * 3 + 1 ];
            float nz = normals[ i * 3 + 2 ];

            // Convert the cartesian vectors to spherical coordinates
            float sx = atan2f( ty, tx ) / 3.14159265f;
            float sy = tz;
            float sz = atan2f( by, bx ) / 3.14159265f;
            float sw = bz;

            sx = sx * 0.5f + 0.5f;
            sy = sy * 0.25f + 0.75f;
            sz = sz * 0.5f + 0.5f;
            sw = sw * 0.25f + 0.75f;
            
            // Convert from float scale to integer scale
            sx = sx * 255.f + 0.5f;
            sy = sy * 255.f + 0.5f;
            sz = sz * 255.f + 0.5f;
            sw = sw * 255.f + 0.5f;

            // Calculate the tangent sign and encode it in the w component
            float n_dot_t_x_b =
                nx * ( ty * bz - tz * by ) +
                ny * ( tz * bx - tx * bz ) +
                nz * ( tx * by - ty * bx );

            out[ i * 4 + 0 ] = static_cast< uint8_t >( sx );
            out[ i * 4 + 1 ] = static_cast< uint8_t >( sy );
            out[ i * 4 + 2 ] = static_cast< uint8_t >( sz );
            out[ i * 4 + 3 ] = static_cast< uint8_t >( sw ) ^ ( n_dot_t_x_b < 0.f ? 0 : 0xFF );
        }
    }
}