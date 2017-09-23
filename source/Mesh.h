#pragma once

#include <DirectXMath.h>

static const unsigned int BONE_WEIGHTS_PER_VERTEX   = 4;
static const unsigned int DEFORM_FACTORS_PER_VERTEX = BONE_WEIGHTS_PER_VERTEX - 1;
static const unsigned int INVALID_INDEX             = 0xFFFFFFFF;

class CMesh
{
public:
    struct SSubMesh
    {
        unsigned int            VertexOffset;
        unsigned int            VertexCount;
        unsigned int            TriangleOffset;
        unsigned int            TriangleCount;
    };
    unsigned int                SubMeshCount;
    SSubMesh*                   SubMeshes;

    unsigned int                VertexCount;
    unsigned int                TriangleCount;

    DirectX::XMFLOAT3*          Positions;
    DirectX::XMFLOAT2*          TextureCoords;
    DirectX::XMFLOAT3*          Normals;
    DirectX::XMFLOAT3*          Tangents;
    DirectX::XMFLOAT3*          Bitangents;
    float*                      BoneWeights;
    unsigned int*               BoneIndices;
    float*                      TangentDeformFactors;
    float*                      BitangentDeformFactors;

    unsigned int*               Indices;

    struct SAnimation
    {
        struct SChannel
        {
            unsigned int        TranslationKeyCount;
            double*             TranslationKeyTimestamps;
            DirectX::XMFLOAT3*  TranslationKeys;

            unsigned int        RotationKeyCount;
            double*             RotationKeyTimestamps;
            DirectX::XMFLOAT4*  RotationKeys;

            unsigned int        ScalingKeyCount;
            double*             ScalingKeyTimestamps;
            DirectX::XMFLOAT3*  ScalingKeys;
        };
        unsigned int            ChannelCount;
        SChannel*               Channels;

        double                  TicksPerSecond;
        double                  Duration;
    };
    unsigned int                AnimationCount;
    SAnimation*                 Animations;

    struct SNode
    {
        DirectX::XMFLOAT4X4     Transformation;

        unsigned int*           AnimationChannels;

        unsigned int            BoneIndex;
        DirectX::XMFLOAT4X4     BoneOffset;

        unsigned int            ChildCount;
        SNode*                  Children;
    };
    SNode                       Root;

    DirectX::XMFLOAT3           BoundingBoxCenter;
    DirectX::XMFLOAT3           BoundingBoxExtent;

    DirectX::XMFLOAT4X4         InverseRootTransformation;
};

CMesh* LoadMesh( const char* filepath );
void DestroyMesh( CMesh* mesh );
void CalculateBoneTransformations( CMesh* mesh, unsigned int animation_index, double animation_time, DirectX::XMFLOAT4X4* bone_transformations );
void UpdateNormalsAndTangents( CMesh* mesh, unsigned int sub_mesh_index, const DirectX::XMFLOAT4X4* bone_transformations );