struct VSInput
{
    float4 position                 : POSITION0;
    float4 bone_weights             : TEXCOORD0;
    uint4  bone_indices             : TEXCOORD1;
    float4 tangents                 : TEXCOORD2;
    float4 deform_factors_tangent   : TEXCOORD3;
    float4 deform_factors_bitangent : TEXCOORD4;
    float3 tangent_ref              : TEXCOORD5;
    float3 bitangent_ref            : TEXCOORD6;
};

struct PSInput
{
    float4 position   : SV_POSITION;
    float3 normal_old : NORMAL0;
    float3 normal_new : NORMAL1;
    float3 normal_ref : NORMAL2;
};

cbuffer Constants : register( b0 )
{
    float       PositionScale;
    float       DeformFactorsTangentScale;
    float       DeformFactorsBitangentScale;
    uint        DiffIntensity;
    float4      ViewDirection;
    float4x4    ViewProjection;
    float4x4    BoneTransformations[ 64 ];
};

float3 Capped( float3 x )
{
    return x * min( 1.0, rsqrt( dot( x, x ) ) * 0.9 );
}

float3 UnpackRGBMSigned( float4 rgbm, float q )
{
    return ( rgbm.rgb * 2.0 - 1.0 ) * rgbm.a * q;
}

float UnpackTangents( float4 tangents, out float3 tangent, out float3 bitangent )
{
    tangents = tangents * 2.0 - 1.0;

    float2 sin_theta, cos_theta;
    sincos( tangents.x * 3.14159265, sin_theta.x, cos_theta.x );
    sincos( tangents.z * 3.14159265, sin_theta.y, cos_theta.y );
    float2 cos_phi = abs( tangents.yw ) * 2.0 - 1.0;
    float2 sin_phi = sqrt( 1.0 - cos_phi * cos_phi );

    tangent   = float3( cos_theta.x * sin_phi.x, sin_theta.x * sin_phi.x, cos_phi.x );
    bitangent = float3( cos_theta.y * sin_phi.y, sin_theta.y * sin_phi.y, cos_phi.y );

    return tangents.w > 0.0 ? 1.0 : -1.0;
}

PSInput VSMain( VSInput input )
{
    PSInput output = ( PSInput )0;

    float4 position = float4( UnpackRGBMSigned( input.position, PositionScale ), 1.0 );

    float3 q0 = mul( BoneTransformations[ input.bone_indices.x ], position ).xyz;
    float3 q1 = mul( BoneTransformations[ input.bone_indices.y ], position ).xyz - q0;
    float3 q2 = mul( BoneTransformations[ input.bone_indices.z ], position ).xyz - q0;
    float3 q3 = mul( BoneTransformations[ input.bone_indices.w ], position ).xyz - q0;

    position = float4( q0 +
                       q1 * input.bone_weights.y +
                       q2 * input.bone_weights.z +
                       q3 * input.bone_weights.w, 1.0 );
    output.position = mul( ViewProjection, position );

    float3x3 bone_matrix =
        input.bone_weights.x * (float3x3)BoneTransformations[ input.bone_indices.x ] +
        input.bone_weights.y * (float3x3)BoneTransformations[ input.bone_indices.y ] +
        input.bone_weights.z * (float3x3)BoneTransformations[ input.bone_indices.z ] +
        input.bone_weights.w * (float3x3)BoneTransformations[ input.bone_indices.w ];
    
    float3 tangent, bitangent;
    float tangent_sign = UnpackTangents( input.tangents, tangent, bitangent );

    tangent = mul( bone_matrix, tangent );
    bitangent = mul( bone_matrix, bitangent );

    output.normal_old = cross( tangent, bitangent ) * tangent_sign;

    float3 deform_factors_tangent = UnpackRGBMSigned( input.deform_factors_tangent, DeformFactorsTangentScale );
    float3 deform_factors_bitangent = UnpackRGBMSigned( input.deform_factors_bitangent, DeformFactorsBitangentScale );
    tangent += Capped( deform_factors_tangent.x * q1 +
                       deform_factors_tangent.y * q2 +
                       deform_factors_tangent.z * q3 );
    bitangent += Capped( deform_factors_bitangent.x * q1 +
                         deform_factors_bitangent.y * q2 +
                         deform_factors_bitangent.z * q3 );
    tangent = normalize( tangent );
    bitangent = normalize( bitangent );

    output.normal_new = cross( tangent, bitangent ) * tangent_sign;

    output.normal_ref = cross( input.tangent_ref, input.bitangent_ref ) * tangent_sign;

    return output;
}

float4 Smooth( float3 normal )
{
    normal = normalize( normal );
    float n_dot_v = saturate( dot( normal, -ViewDirection.xyz ) );
    return float4( n_dot_v, n_dot_v, n_dot_v, 1.0 );
}

float4 Diff( float3 normal_a, float3 normal_b )
{
    normal_a = normalize( normal_a );
    normal_b = normalize( normal_b );
    float a_dot_v = saturate( dot( normal_a, -ViewDirection.xyz ) );
    float a_dot_b = saturate( dot( normal_a, normal_b ) );
    a_dot_b = pow( a_dot_b, float( DiffIntensity ) );
    return float4( a_dot_v * float3( 1.0, a_dot_b, a_dot_b ), 1.0 );
}

float4 PSMain( PSInput input ) : SV_Target
{
#if PS_OUT == 0
    return Smooth( input.normal_old );
#elif PS_OUT == 1
    return Smooth( input.normal_new );
#elif PS_OUT == 2
    return Smooth( input.normal_ref );
#elif PS_OUT == 3
    return Diff( input.normal_old, input.normal_new );
#elif PS_OUT == 4
    return Diff( input.normal_old, input.normal_ref );
#elif PS_OUT == 5
    return Diff( input.normal_new, input.normal_ref );
#else
    #error
#endif
}