#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <unordered_map>

void CalculateNormalsAndTangents( CMesh* mesh, unsigned int sub_mesh_index, const DirectX::XMFLOAT3* sub_mesh_positions )
{
    const CMesh::SSubMesh sub_mesh = mesh->SubMeshes[ sub_mesh_index ];

    memset( mesh->Normals + sub_mesh.VertexOffset, 0, sub_mesh.VertexCount * sizeof( DirectX::XMFLOAT3 ) );
    memset( mesh->Tangents + sub_mesh.VertexOffset, 0, sub_mesh.VertexCount * sizeof( DirectX::XMFLOAT3 ) );
    memset( mesh->Bitangents + sub_mesh.VertexOffset, 0, sub_mesh.VertexCount * sizeof( DirectX::XMFLOAT3 ) );

    for ( unsigned int j = 0; j < sub_mesh.TriangleCount; ++j )
    {
        unsigned int sub_mesh_indices[ 3 ];
        sub_mesh_indices[ 0 ] = mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 0 ];
        sub_mesh_indices[ 1 ] = mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 1 ];
        sub_mesh_indices[ 2 ] = mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 2 ];

        unsigned int indices[ 3 ];
        indices[ 0 ] = sub_mesh_indices[ 0 ] + sub_mesh.VertexOffset;
        indices[ 1 ] = sub_mesh_indices[ 1 ] + sub_mesh.VertexOffset;
        indices[ 2 ] = sub_mesh_indices[ 2 ] + sub_mesh.VertexOffset;

        DirectX::XMVECTOR texture_coords[ 3 ];
        texture_coords[ 0 ] = DirectX::XMLoadFloat2( &mesh->TextureCoords[ indices[ 0 ] ] );
        texture_coords[ 1 ] = DirectX::XMLoadFloat2( &mesh->TextureCoords[ indices[ 1 ] ] );
        texture_coords[ 2 ] = DirectX::XMLoadFloat2( &mesh->TextureCoords[ indices[ 2 ] ] );

        DirectX::XMVECTOR u0 = DirectX::XMVectorSubtract( texture_coords[ 1 ], texture_coords[ 0 ] );
        DirectX::XMVECTOR u1 = DirectX::XMVectorSubtract( texture_coords[ 2 ], texture_coords[ 0 ] );

        float uv_area = DirectX::XMVectorGetX( DirectX::XMVector2Cross( u0, u1 ) );
        if ( uv_area == 0 )
            continue;
        float s0 = -DirectX::XMVectorGetX( u1 ) / uv_area;
        float s1 = DirectX::XMVectorGetY( u1 ) / uv_area;
        float t0 = DirectX::XMVectorGetX( u0 ) / uv_area;
        float t1 = -DirectX::XMVectorGetY( u0 ) / uv_area;

        DirectX::XMVECTOR positions[ 3 ];
        positions[ 0 ] = DirectX::XMLoadFloat3( &sub_mesh_positions[ sub_mesh_indices[ 0 ] ] );
        positions[ 1 ] = DirectX::XMLoadFloat3( &sub_mesh_positions[ sub_mesh_indices[ 1 ] ] );
        positions[ 2 ] = DirectX::XMLoadFloat3( &sub_mesh_positions[ sub_mesh_indices[ 2 ] ] );

        DirectX::XMVECTOR e0 = DirectX::XMVectorSubtract( positions[ 1 ], positions[ 0 ] );
        DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract( positions[ 2 ], positions[ 0 ] );

        DirectX::XMVECTOR triangle_normal = DirectX::XMVector3Cross( e1, e0 );
        float triangle_area = DirectX::XMVectorGetX( DirectX::XMVector3Length( triangle_normal ) );

        DirectX::XMVECTOR triangle_tangent = DirectX::XMVector3Normalize( DirectX::XMVectorAdd( DirectX::XMVectorScale( e0, s0 ), DirectX::XMVectorScale( e1, t0 ) ) );
        DirectX::XMVECTOR triangle_bitangent = DirectX::XMVector3Normalize( DirectX::XMVectorAdd( DirectX::XMVectorScale( e0, s1 ), DirectX::XMVectorScale( e1, t1 ) ) );

        for ( unsigned int k = 0; k < 3; ++k )
        {
            DirectX::XMVECTOR e2 = DirectX::XMVectorSubtract( positions[ ( k + 1 ) % 3 ], positions[ k ] );
            DirectX::XMVECTOR e3 = DirectX::XMVectorSubtract( positions[ ( k + 2 ) % 3 ], positions[ k ] );
            float wedge_angle = DirectX::XMVectorGetX( DirectX::XMVector3AngleBetweenVectors( e2, e3 ) ) * triangle_area;

            DirectX::XMVECTOR vertex_normal = DirectX::XMLoadFloat3( &mesh->Normals[ indices[ k ] ] );
            DirectX::XMVECTOR vertex_tangent = DirectX::XMLoadFloat3( &mesh->Tangents[ indices[ k ] ] );
            DirectX::XMVECTOR vertex_bitangent = DirectX::XMLoadFloat3( &mesh->Bitangents[ indices[ k ] ] );
            
            vertex_normal = DirectX::XMVectorAdd( vertex_normal, DirectX::XMVectorScale( triangle_normal, wedge_angle ) );
            vertex_tangent = DirectX::XMVectorAdd( vertex_tangent, DirectX::XMVectorScale( triangle_tangent, wedge_angle ) );
            vertex_bitangent = DirectX::XMVectorAdd( vertex_bitangent, DirectX::XMVectorScale( triangle_bitangent, wedge_angle ) );
            
            DirectX::XMStoreFloat3( &mesh->Normals[ indices[ k ] ], vertex_normal );
            DirectX::XMStoreFloat3( &mesh->Tangents[ indices[ k ] ], vertex_tangent );
            DirectX::XMStoreFloat3( &mesh->Bitangents[ indices[ k ] ], vertex_bitangent );
        }
    }

    for ( unsigned int i = sub_mesh.VertexOffset; i < ( sub_mesh.VertexOffset + sub_mesh.VertexCount ); ++i )
    {
        DirectX::XMVECTOR normal = DirectX::XMLoadFloat3( &mesh->Normals[ i ] );
        DirectX::XMVECTOR tangent = DirectX::XMLoadFloat3( &mesh->Tangents[ i ] );
        DirectX::XMVECTOR bitangent = DirectX::XMLoadFloat3( &mesh->Bitangents[ i ] );

        normal = DirectX::XMVector3Normalize( normal );

        DirectX::XMStoreFloat3( &mesh->Normals[ i ], normal );
        DirectX::XMStoreFloat3( &mesh->Tangents[ i ], DirectX::XMVector3Normalize( DirectX::XMVector3Cross( DirectX::XMVector3Cross( normal, bitangent ), normal ) ) );
        DirectX::XMStoreFloat3( &mesh->Bitangents[ i ], DirectX::XMVector3Normalize( DirectX::XMVector3Cross( DirectX::XMVector3Cross( normal, tangent ), normal ) ) );
    }
}
void UpdateNormalsAndTangents( CMesh* mesh, unsigned int sub_mesh_index, const DirectX::XMFLOAT4X4* bone_transformations )
{
    const CMesh::SSubMesh sub_mesh = mesh->SubMeshes[ sub_mesh_index ];

    DirectX::XMFLOAT3* sub_mesh_positions = new DirectX::XMFLOAT3[ sub_mesh.VertexCount ];
    for ( unsigned int i = 0; i < sub_mesh.VertexCount; ++i )
    {
        DirectX::XMVECTOR base_position = DirectX::XMLoadFloat3( &mesh->Positions[ sub_mesh.VertexOffset + i ] );

        float* bone_weights = mesh->BoneWeights + ( sub_mesh.VertexOffset + i ) * BONE_WEIGHTS_PER_VERTEX;
        unsigned int* bone_indices = mesh->BoneIndices + ( sub_mesh.VertexOffset + i ) * BONE_WEIGHTS_PER_VERTEX;

        DirectX::XMVECTOR q0 = DirectX::XMVector3TransformCoord( base_position, DirectX::XMLoadFloat4x4( &bone_transformations[ bone_indices[ 0 ] ] ) );
        DirectX::XMVECTOR skin_position = q0;
        for ( unsigned int j = 1; j < BONE_WEIGHTS_PER_VERTEX; ++j )
        {
            DirectX::XMVECTOR q = DirectX::XMVectorSubtract( DirectX::XMVector3TransformCoord( base_position, DirectX::XMLoadFloat4x4( &bone_transformations[ bone_indices[ j ] ] ) ), q0 );
            skin_position = DirectX::XMVectorAdd( skin_position, DirectX::XMVectorScale( q, bone_weights[ j ] ) );
        }
        DirectX::XMStoreFloat3( &sub_mesh_positions[ i ], skin_position );
    }

    CalculateNormalsAndTangents( mesh, sub_mesh_index, sub_mesh_positions );

    delete[] sub_mesh_positions;
}

void CreateNodeHierarchy( const aiScene* scene, const aiNode* node, CMesh::SNode& mesh_node, const std::unordered_map<std::string, unsigned int>& bone_index_map, const std::vector<DirectX::XMFLOAT4X4>& bone_offsets )
{
    std::string node_name( node->mName.C_Str() );

    aiMatrix4x4 transformation = node->mTransformation;
    transformation.Transpose();
    mesh_node.Transformation = DirectX::XMFLOAT4X4( &transformation.a1 );

    mesh_node.AnimationChannels = new unsigned int[ scene->mNumAnimations ];
    for ( unsigned int i = 0; i < scene->mNumAnimations; ++i )
    {
        mesh_node.AnimationChannels[ i ] = INVALID_INDEX;
        for ( unsigned int j = 0; j < scene->mAnimations[ i ]->mNumChannels; ++j )
        {
            if ( scene->mAnimations[ i ]->mChannels[ j ]->mNodeName == node->mName )
            {
                mesh_node.AnimationChannels[ i ] = j;
                break;
            }
        }
    }

    if ( bone_index_map.find( node_name ) != bone_index_map.end() )
    {
        mesh_node.BoneIndex = bone_index_map.at( node_name );
        mesh_node.BoneOffset = bone_offsets[ mesh_node.BoneIndex ];
    }
    else
    {
        mesh_node.BoneIndex = INVALID_INDEX;
        DirectX::XMStoreFloat4x4( &mesh_node.BoneOffset, DirectX::XMMatrixIdentity() );
    }

    mesh_node.ChildCount = node->mNumChildren;
    mesh_node.Children = new CMesh::SNode[ mesh_node.ChildCount ];
    for ( unsigned int i = 0; i < mesh_node.ChildCount; ++i )
    {
        CreateNodeHierarchy( scene, node->mChildren[ i ], mesh_node.Children[ i ], bone_index_map, bone_offsets );
    }
}
void DestroyNodeHierarchy( CMesh::SNode& mesh_node )
{
    for ( unsigned int i = 0; i < mesh_node.ChildCount; ++i )
    {
        DestroyNodeHierarchy( mesh_node.Children[ i ] );
    }

    delete[] mesh_node.AnimationChannels;
    delete[] mesh_node.Children;
}

void CalculateBoundingBox( const aiScene* scene, const aiNode* node, DirectX::XMMATRIX parent_transformation, DirectX::XMVECTOR& bounding_box_min, DirectX::XMVECTOR& bounding_box_max )
{
    DirectX::XMMATRIX local_node_transformation = DirectX::XMMatrixTranspose( DirectX::XMLoadFloat4x4( reinterpret_cast< const DirectX::XMFLOAT4X4* >( &node->mTransformation ) ) );
    DirectX::XMMATRIX global_node_transformation = local_node_transformation * parent_transformation;

    for ( unsigned int i = 0; i < node->mNumMeshes; ++i )
    {
        aiMesh* mesh = scene->mMeshes[ node->mMeshes[ i ] ];

        for ( unsigned int j = 0; j < mesh->mNumVertices; ++j )
        {
            DirectX::XMVECTOR position = DirectX::XMLoadFloat3( reinterpret_cast< DirectX::XMFLOAT3* >( &mesh->mVertices[ j ] ) );
            position = DirectX::XMVector3TransformCoord( position, global_node_transformation );

            bounding_box_min = DirectX::XMVectorMin( bounding_box_min, position );
            bounding_box_max = DirectX::XMVectorMax( bounding_box_max, position );
        }
    }

    for ( unsigned int i = 0; i < node->mNumChildren; ++i )
    {
        CalculateBoundingBox( scene, node->mChildren[ i ], global_node_transformation, bounding_box_min, bounding_box_max );
    }
}

unsigned int FindBoneWeightIndex( unsigned int bone_index, unsigned int* bone_indices )
{
    for ( unsigned int i = 0; i < BONE_WEIGHTS_PER_VERTEX; ++i )
    {
        if ( bone_index == bone_indices[ i ] )
        {
            return i;
        }
    }
    return INVALID_INDEX;
}
float GetBoneWeight( unsigned int bone_weight_index, float* bone_weights )
{
    return bone_weight_index == INVALID_INDEX ? 0 : bone_weights[ bone_weight_index ];
}

CMesh* LoadMesh( const char* filepath )
{
    CMesh* mesh = new CMesh();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( filepath, aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs );
    assert( scene != nullptr && scene->mNumMeshes > 0 );

    mesh->VertexCount = 0;
    mesh->TriangleCount = 0;

    mesh->SubMeshCount = scene->mNumMeshes;
    mesh->SubMeshes = new CMesh::SSubMesh[ mesh->SubMeshCount ];
    for ( unsigned int i = 0; i < mesh->SubMeshCount; ++i )
    {
        mesh->SubMeshes[ i ].VertexOffset = mesh->VertexCount;
        mesh->SubMeshes[ i ].VertexCount = scene->mMeshes[ i ]->mNumVertices;
        mesh->SubMeshes[ i ].TriangleOffset = mesh->TriangleCount;
        mesh->SubMeshes[ i ].TriangleCount = scene->mMeshes[ i ]->mNumFaces;

        mesh->VertexCount += scene->mMeshes[ i ]->mNumVertices;
        mesh->TriangleCount += scene->mMeshes[ i ]->mNumFaces;
    }

    mesh->Positions = new DirectX::XMFLOAT3[ mesh->VertexCount ];
    mesh->TextureCoords = new DirectX::XMFLOAT2[ mesh->VertexCount ];
    mesh->Normals = new DirectX::XMFLOAT3[ mesh->VertexCount ];
    mesh->Tangents = new DirectX::XMFLOAT3[ mesh->VertexCount ];
    mesh->Bitangents = new DirectX::XMFLOAT3[ mesh->VertexCount ];
    mesh->BoneWeights = new float[ mesh->VertexCount * BONE_WEIGHTS_PER_VERTEX ];
    mesh->BoneIndices = new unsigned int[ mesh->VertexCount * BONE_WEIGHTS_PER_VERTEX ];
    mesh->TangentDeformFactors = new float[ mesh->VertexCount * DEFORM_FACTORS_PER_VERTEX ];
    mesh->BitangentDeformFactors = new float[ mesh->VertexCount * DEFORM_FACTORS_PER_VERTEX ];
    memset( mesh->BoneWeights, 0, mesh->VertexCount * BONE_WEIGHTS_PER_VERTEX * sizeof( float ) );
    memset( mesh->BoneIndices, 0, mesh->VertexCount * BONE_WEIGHTS_PER_VERTEX * sizeof( unsigned int ) );
    memset( mesh->TangentDeformFactors, 0, mesh->VertexCount * DEFORM_FACTORS_PER_VERTEX * sizeof( float ) );
    memset( mesh->BitangentDeformFactors, 0, mesh->VertexCount * DEFORM_FACTORS_PER_VERTEX * sizeof( float ) );

    mesh->Indices = new unsigned int[ mesh->TriangleCount * 3 ];

    std::unordered_map<std::string, unsigned int> bone_index_map;
    std::vector<DirectX::XMFLOAT4X4> bone_offsets;

    for ( unsigned int i = 0; i < mesh->SubMeshCount; ++i )
    {
        assert( scene->mMeshes[ i ]->HasPositions() &&
                scene->mMeshes[ i ]->HasTextureCoords( 0 ) &&
                scene->mMeshes[ i ]->HasNormals() &&
                scene->mMeshes[ i ]->HasBones() &&
                scene->mMeshes[ i ]->HasFaces() );

        const CMesh::SSubMesh sub_mesh = mesh->SubMeshes[ i ];

        for ( unsigned int j = 0; j < sub_mesh.VertexCount; ++j )
        {
            mesh->Positions[ sub_mesh.VertexOffset + j ].x = scene->mMeshes[ i ]->mVertices[ j ].x;
            mesh->Positions[ sub_mesh.VertexOffset + j ].y = scene->mMeshes[ i ]->mVertices[ j ].y;
            mesh->Positions[ sub_mesh.VertexOffset + j ].z = scene->mMeshes[ i ]->mVertices[ j ].z;
        }
        for ( unsigned int j = 0; j < sub_mesh.VertexCount; ++j )
        {
            mesh->TextureCoords[ sub_mesh.VertexOffset + j ].x = scene->mMeshes[ i ]->mTextureCoords[ 0 ][ j ].x;
            mesh->TextureCoords[ sub_mesh.VertexOffset + j ].y = scene->mMeshes[ i ]->mTextureCoords[ 0 ][ j ].y;
        }
        if ( scene->mMeshes[ i ]->HasBones() )
        {
            for ( unsigned int j = 0; j < scene->mMeshes[ i ]->mNumBones; ++j )
            {
                unsigned int bone_index = 0;
                std::string bone_name = std::string( scene->mMeshes[ i ]->mBones[ j ]->mName.C_Str() );
                if ( bone_index_map.find( bone_name ) == bone_index_map.end() )
                {
                    bone_index = static_cast< unsigned int >( bone_index_map.size() );
                    bone_index_map[ bone_name ] = bone_index;

                    aiMatrix4x4 offset_matrix = scene->mMeshes[ i ]->mBones[ j ]->mOffsetMatrix;
                    offset_matrix.Transpose();
                    bone_offsets.push_back( DirectX::XMFLOAT4X4( &offset_matrix.a1 ) );
                }
                else
                {
                    bone_index = bone_index_map[ bone_name ];
                }

                for ( unsigned int k = 0; k < scene->mMeshes[ i ]->mBones[ j ]->mNumWeights; ++k )
                {
                    unsigned int vertex_index = scene->mMeshes[ i ]->mBones[ j ]->mWeights[ k ].mVertexId;
                    float bone_weight = scene->mMeshes[ i ]->mBones[ j ]->mWeights[ k ].mWeight;
                    assert( bone_weight != 0 );

                    float* bone_weights = mesh->BoneWeights + ( sub_mesh.VertexOffset + vertex_index ) * BONE_WEIGHTS_PER_VERTEX;
                    unsigned int* bone_indices = mesh->BoneIndices + ( sub_mesh.VertexOffset + vertex_index ) * BONE_WEIGHTS_PER_VERTEX;

                    for ( unsigned int l = 0; l < BONE_WEIGHTS_PER_VERTEX; ++l )
                    {
                        if ( bone_weights[ l ] == 0 )
                        {
                            bone_weights[ l ] = bone_weight;
                            bone_indices[ l ] = bone_index;
                            break;
                        }
                    }
                }
            }
            for ( unsigned int j = 0; j < sub_mesh.VertexCount; ++j )
            {
                float* bone_weights = mesh->BoneWeights + ( sub_mesh.VertexOffset + j ) * BONE_WEIGHTS_PER_VERTEX;
                unsigned int* bone_indices = mesh->BoneIndices + ( sub_mesh.VertexOffset + j ) * BONE_WEIGHTS_PER_VERTEX;

                // Make sure the sum of all weights is 1
                float bone_weight_sum = 0;
                for ( unsigned int k = 0; k < BONE_WEIGHTS_PER_VERTEX; ++k )
                {
                    bone_weight_sum += bone_weights[ k ];
                }
                if ( bone_weight_sum == 0 )
                {
                    bone_weights[ 0 ] = 1.0f;
                }
                else
                {
                    for ( unsigned int k = 0; k < BONE_WEIGHTS_PER_VERTEX; ++k )
                    {
                        bone_weights[ k ] /= bone_weight_sum;
                    }
                }

                // Sort bone weights
                for ( unsigned int k = 1; k < BONE_WEIGHTS_PER_VERTEX; ++k )
                {
                    for ( unsigned int l = k; l > 0 && bone_weights[ l - 1 ] < bone_weights[ l ]; --l )
                    {
                        std::swap( bone_weights[ l - 1 ], bone_weights[ l ] );
                        std::swap( bone_indices[ l - 1 ], bone_indices[ l ] );
                    }
                }
            }
        }
        for ( unsigned int j = 0; j < sub_mesh.TriangleCount; ++j )
        {
            assert( scene->mMeshes[ i ]->mFaces[ j ].mNumIndices == 3 );
            mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 0 ] = scene->mMeshes[ i ]->mFaces[ j ].mIndices[ 0 ];
            mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 1 ] = scene->mMeshes[ i ]->mFaces[ j ].mIndices[ 1 ];
            mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 2 ] = scene->mMeshes[ i ]->mFaces[ j ].mIndices[ 2 ];
        }

        CalculateNormalsAndTangents( mesh, i, mesh->Positions + sub_mesh.VertexOffset );
    }

    // Calculate deform factors
    std::vector<float> deform_factor_sums( mesh->VertexCount * DEFORM_FACTORS_PER_VERTEX, 0 );
    for ( unsigned int i = 0; i < mesh->SubMeshCount; ++i )
    {
        const CMesh::SSubMesh sub_mesh = mesh->SubMeshes[ i ];

        for ( unsigned int j = 0; j < sub_mesh.TriangleCount; ++j )
        {
            unsigned int indices[ 3 ];
            indices[ 0 ] = mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 0 ] + sub_mesh.VertexOffset;
            indices[ 1 ] = mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 1 ] + sub_mesh.VertexOffset;
            indices[ 2 ] = mesh->Indices[ ( sub_mesh.TriangleOffset + j ) * 3 + 2 ] + sub_mesh.VertexOffset;

            DirectX::XMVECTOR positions[ 3 ];
            positions[ 0 ] = DirectX::XMLoadFloat3( &mesh->Positions[ indices[ 0 ] ] );
            positions[ 1 ] = DirectX::XMLoadFloat3( &mesh->Positions[ indices[ 1 ] ] );
            positions[ 2 ] = DirectX::XMLoadFloat3( &mesh->Positions[ indices[ 2 ] ] );

            DirectX::XMVECTOR e0 = DirectX::XMVectorSubtract( positions[ 1 ], positions[ 0 ] );
            DirectX::XMVECTOR e1 = DirectX::XMVectorSubtract( positions[ 2 ], positions[ 0 ] );
            DirectX::XMVECTOR n = DirectX::XMVector3Cross( e1, e0 );

            float triangle_area = DirectX::XMVectorGetX( DirectX::XMVector3Length( n ) );

            DirectX::XMMATRIX local_to_triangle = DirectX::XMLoadFloat3x3( &DirectX::XMFLOAT3X3(
                DirectX::XMVectorGetX( e0 ), DirectX::XMVectorGetX( e1 ), DirectX::XMVectorGetX( n ),
                DirectX::XMVectorGetY( e0 ), DirectX::XMVectorGetY( e1 ), DirectX::XMVectorGetY( n ),
                DirectX::XMVectorGetZ( e0 ), DirectX::XMVectorGetZ( e1 ), DirectX::XMVectorGetZ( n ) ) );

            DirectX::XMVECTOR determinant;
            DirectX::XMMATRIX triangle_to_local = DirectX::XMMatrixInverse( &determinant, local_to_triangle );

            std::vector<bool> bone_weights_done( bone_offsets.size(), false );
            for ( unsigned int k = 0; k < 3; ++k )
            {
                for ( unsigned int l = 0; l < BONE_WEIGHTS_PER_VERTEX; ++l )
                {
                    if ( mesh->BoneWeights[ indices[ k ] * BONE_WEIGHTS_PER_VERTEX + l ] == 0 )
                        continue;

                    unsigned int bone_index = mesh->BoneIndices[ indices[ k ] * BONE_WEIGHTS_PER_VERTEX + l ];

                    if ( bone_weights_done[ bone_index ] )
                        continue;
                    bone_weights_done[ bone_index ] = true;

                    unsigned int weight_indices[ 3 ];
                    weight_indices[ 0 ] = FindBoneWeightIndex( bone_index, mesh->BoneIndices + indices[ 0 ] * BONE_WEIGHTS_PER_VERTEX );
                    weight_indices[ 1 ] = FindBoneWeightIndex( bone_index, mesh->BoneIndices + indices[ 1 ] * BONE_WEIGHTS_PER_VERTEX );
                    weight_indices[ 2 ] = FindBoneWeightIndex( bone_index, mesh->BoneIndices + indices[ 2 ] * BONE_WEIGHTS_PER_VERTEX );

                    float w0 = GetBoneWeight( weight_indices[ 0 ], mesh->BoneWeights + indices[ 0 ] * BONE_WEIGHTS_PER_VERTEX );
                    float w1 = GetBoneWeight( weight_indices[ 1 ], mesh->BoneWeights + indices[ 1 ] * BONE_WEIGHTS_PER_VERTEX );
                    float w2 = GetBoneWeight( weight_indices[ 2 ], mesh->BoneWeights + indices[ 2 ] * BONE_WEIGHTS_PER_VERTEX );

                    float dw0 = w1 - w0;
                    float dw1 = w2 - w0;

                    if ( dw0 == 0 && dw1 == 0 )
                        continue;

                    DirectX::XMVECTOR weight_gradient = DirectX::XMVector3Transform( DirectX::XMVectorSet( dw0, dw1, 0, 0 ), triangle_to_local );

                    for ( unsigned int m = 0; m < 3; ++m )
                    {
                        unsigned int weight_index = weight_indices[ m ];
                        if ( weight_index != INVALID_INDEX && weight_index > 0 )
                        {
                            DirectX::XMVECTOR e2 = DirectX::XMVectorSubtract( positions[ ( m + 1 ) % 3 ], positions[ m ] );
                            DirectX::XMVECTOR e3 = DirectX::XMVectorSubtract( positions[ ( m + 2 ) % 3 ], positions[ m ] );
                            float wedge_angle = DirectX::XMVectorGetX( DirectX::XMVector3AngleBetweenVectors( e2, e3 ) ) * triangle_area;

                            DirectX::XMVECTOR tangent = DirectX::XMLoadFloat3( mesh->Tangents + indices[ m ] );
                            DirectX::XMVECTOR bitangent = DirectX::XMLoadFloat3( mesh->Bitangents + indices[ m ] );

                            unsigned int deform_factor_index = indices[ m ] * DEFORM_FACTORS_PER_VERTEX + weight_index - 1;
                            mesh->TangentDeformFactors[ deform_factor_index ] += DirectX::XMVectorGetX( DirectX::XMVector3Dot( tangent, weight_gradient ) ) * wedge_angle;
                            mesh->BitangentDeformFactors[ deform_factor_index ] += DirectX::XMVectorGetX( DirectX::XMVector3Dot( bitangent, weight_gradient ) ) * wedge_angle;
                            deform_factor_sums[ deform_factor_index ] += wedge_angle;
                        }
                    }
                }
            }
        }
    }
    for ( unsigned int i = 0; i < mesh->VertexCount * DEFORM_FACTORS_PER_VERTEX; ++i )
    {
        if ( deform_factor_sums[ i ] > 0 )
        {
            mesh->TangentDeformFactors[ i ] /= deform_factor_sums[ i ];
            mesh->BitangentDeformFactors[ i ] /= deform_factor_sums[ i ];
        }
    }

    mesh->AnimationCount = scene->mNumAnimations;
    mesh->Animations = new CMesh::SAnimation[ mesh->AnimationCount ];
    for ( unsigned int i = 0; i < mesh->AnimationCount; ++i )
    {
        CMesh::SAnimation& animation = mesh->Animations[ i ];

        animation.ChannelCount = scene->mAnimations[ i ]->mNumChannels;
        animation.Channels = new CMesh::SAnimation::SChannel[ animation.ChannelCount ];
        for ( unsigned int j = 0; j < animation.ChannelCount; ++j )
        {
            CMesh::SAnimation::SChannel& channel = animation.Channels[ j ];

            channel.TranslationKeyCount = scene->mAnimations[ i ]->mChannels[ j ]->mNumPositionKeys;
            channel.TranslationKeyTimestamps = new double[ channel.TranslationKeyCount ];
            channel.TranslationKeys = new DirectX::XMFLOAT3[ channel.TranslationKeyCount ];
            for ( unsigned int k = 0; k < channel.TranslationKeyCount; ++k )
            {
                channel.TranslationKeyTimestamps[ k ] = scene->mAnimations[ i ]->mChannels[ j ]->mPositionKeys[ k ].mTime;
                channel.TranslationKeys[ k ].x = scene->mAnimations[ i ]->mChannels[ j ]->mPositionKeys[ k ].mValue.x;
                channel.TranslationKeys[ k ].y = scene->mAnimations[ i ]->mChannels[ j ]->mPositionKeys[ k ].mValue.y;
                channel.TranslationKeys[ k ].z = scene->mAnimations[ i ]->mChannels[ j ]->mPositionKeys[ k ].mValue.z;
            }

            channel.RotationKeyCount = scene->mAnimations[ i ]->mChannels[ j ]->mNumRotationKeys;
            channel.RotationKeyTimestamps = new double[ channel.RotationKeyCount ];
            channel.RotationKeys = new DirectX::XMFLOAT4[ channel.RotationKeyCount ];
            for ( unsigned int k = 0; k < channel.RotationKeyCount; ++k )
            {
                channel.RotationKeyTimestamps[ k ] = scene->mAnimations[ i ]->mChannels[ j ]->mRotationKeys[ k ].mTime;
                channel.RotationKeys[ k ].x = scene->mAnimations[ i ]->mChannels[ j ]->mRotationKeys[ k ].mValue.x;
                channel.RotationKeys[ k ].y = scene->mAnimations[ i ]->mChannels[ j ]->mRotationKeys[ k ].mValue.y;
                channel.RotationKeys[ k ].z = scene->mAnimations[ i ]->mChannels[ j ]->mRotationKeys[ k ].mValue.z;
                channel.RotationKeys[ k ].w = scene->mAnimations[ i ]->mChannels[ j ]->mRotationKeys[ k ].mValue.w;
            }

            channel.ScalingKeyCount = scene->mAnimations[ i ]->mChannels[ j ]->mNumScalingKeys;
            channel.ScalingKeyTimestamps = new double[ channel.ScalingKeyCount ];
            channel.ScalingKeys = new DirectX::XMFLOAT3[ channel.ScalingKeyCount ];
            for ( unsigned int k = 0; k < channel.ScalingKeyCount; ++k )
            {
                channel.ScalingKeyTimestamps[ k ] = scene->mAnimations[ i ]->mChannels[ j ]->mScalingKeys[ k ].mTime;
                channel.ScalingKeys[ k ].x = scene->mAnimations[ i ]->mChannels[ j ]->mScalingKeys[ k ].mValue.x;
                channel.ScalingKeys[ k ].y = scene->mAnimations[ i ]->mChannels[ j ]->mScalingKeys[ k ].mValue.y;
                channel.ScalingKeys[ k ].z = scene->mAnimations[ i ]->mChannels[ j ]->mScalingKeys[ k ].mValue.z;
            }
        }

        animation.TicksPerSecond = scene->mAnimations[ i ]->mTicksPerSecond;
        animation.Duration = scene->mAnimations[ i ]->mDuration;
    }

    CreateNodeHierarchy( scene, scene->mRootNode, mesh->Root, bone_index_map, bone_offsets );

    DirectX::XMVECTOR bounding_box_min = DirectX::XMVectorSet( FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX );
    DirectX::XMVECTOR bounding_box_max = DirectX::XMVectorSet( -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX );
    CalculateBoundingBox( scene, scene->mRootNode, DirectX::XMMatrixIdentity(), bounding_box_min, bounding_box_max );
    DirectX::XMStoreFloat3( &mesh->BoundingBoxCenter, DirectX::XMVectorScale( DirectX::XMVectorAdd( bounding_box_max, bounding_box_min ), 0.5f ) );
    DirectX::XMStoreFloat3( &mesh->BoundingBoxExtent, DirectX::XMVectorScale( DirectX::XMVectorSubtract( bounding_box_max, bounding_box_min ), 0.5f ) );

    aiMatrix4x4 root_transformation = scene->mRootNode->mTransformation;
    root_transformation.Inverse();
    root_transformation.Transpose();
    mesh->InverseRootTransformation = DirectX::XMFLOAT4X4( &root_transformation.a1 );

    return mesh;
}

void DestroyMesh( CMesh* mesh )
{
    DestroyNodeHierarchy( mesh->Root );

    for ( unsigned int i = 0; i < mesh->AnimationCount; ++i )
    {
        const CMesh::SAnimation& animation = mesh->Animations[ i ];
        for ( unsigned int j = 0; j < animation.ChannelCount; ++j )
        {
            const CMesh::SAnimation::SChannel& channel = animation.Channels[ j ];
            delete[] channel.TranslationKeyTimestamps;
            delete[] channel.TranslationKeys;
            delete[] channel.RotationKeyTimestamps;
            delete[] channel.RotationKeys;
            delete[] channel.ScalingKeyTimestamps;
            delete[] channel.ScalingKeys;
        }
        delete[] animation.Channels;
    }
    delete[] mesh->Animations;

    delete[] mesh->Indices;

    delete[] mesh->BitangentDeformFactors;
    delete[] mesh->TangentDeformFactors;
    delete[] mesh->BoneIndices;
    delete[] mesh->BoneWeights;
    delete[] mesh->Bitangents;
    delete[] mesh->Tangents;
    delete[] mesh->Normals;
    delete[] mesh->TextureCoords;
    delete[] mesh->Positions;

    delete[] mesh->SubMeshes;

    delete mesh;
    mesh = nullptr;
}

void CalculateBoneTransformations( const CMesh::SNode& mesh_node, const CMesh::SAnimation& animation, unsigned int animation_index, double animation_time, DirectX::XMMATRIX parent_transformation, DirectX::XMMATRIX inverse_root_transformation, DirectX::XMFLOAT4X4* bone_transformations )
{
    DirectX::XMMATRIX local_node_transformation = DirectX::XMLoadFloat4x4( &mesh_node.Transformation );

    const int channel_index = mesh_node.AnimationChannels[ animation_index ];
    if ( channel_index != INVALID_INDEX )
    {
        const CMesh::SAnimation::SChannel channel = animation.Channels[ channel_index ];

        DirectX::XMVECTOR scaling;
        if ( channel.ScalingKeyCount == 1 )
        {
            scaling = DirectX::XMLoadFloat3( &channel.ScalingKeys[ 0 ] );
        }
        else
        {
            unsigned int curr_index = channel.ScalingKeyCount - 1;
            for ( unsigned int i = 0; i < channel.ScalingKeyCount - 1; ++i )
            {
                if ( animation_time <= channel.ScalingKeyTimestamps[ i + 1 ] )
                {
                    curr_index = i;
                    break;
                }
            }
            unsigned int next_index = std::min( curr_index + 1, channel.ScalingKeyCount - 1 );

            double curr_time = channel.ScalingKeyTimestamps[ curr_index ];
            double next_time = channel.ScalingKeyTimestamps[ next_index ];
            float t = static_cast< float >( ( animation_time - curr_time ) / ( next_time - curr_time ) );

            DirectX::XMVECTOR curr_scaling = DirectX::XMLoadFloat3( &channel.ScalingKeys[ curr_index ] );
            DirectX::XMVECTOR next_scaling = DirectX::XMLoadFloat3( &channel.ScalingKeys[ next_index ] );
            scaling = DirectX::XMVectorLerp( curr_scaling, next_scaling, t );
        }

        DirectX::XMVECTOR rotation;
        if ( channel.RotationKeyCount == 1 )
        {
            rotation = DirectX::XMLoadFloat4( &channel.RotationKeys[ 0 ] );
        }
        else
        {
            unsigned int curr_index = channel.RotationKeyCount - 1;
            for ( unsigned int i = 0; i < channel.RotationKeyCount - 1; ++i )
            {
                if ( animation_time <= channel.RotationKeyTimestamps[ i + 1 ] )
                {
                    curr_index = i;
                    break;
                }
            }
            unsigned int next_index = std::min( curr_index + 1, channel.RotationKeyCount - 1 );

            double curr_time = channel.RotationKeyTimestamps[ curr_index ];
            double next_time = channel.RotationKeyTimestamps[ next_index ];
            float t = static_cast< float >( ( animation_time - curr_time ) / ( next_time - curr_time ) );

            DirectX::XMVECTOR curr_rotation = DirectX::XMLoadFloat4( &channel.RotationKeys[ curr_index ] );
            DirectX::XMVECTOR next_rotation = DirectX::XMLoadFloat4( &channel.RotationKeys[ next_index ] );
            rotation = DirectX::XMQuaternionNormalize( DirectX::XMQuaternionSlerp( curr_rotation, next_rotation, t ) );
        }

        DirectX::XMVECTOR translation;
        if ( channel.TranslationKeyCount == 1 )
        {
            translation = DirectX::XMLoadFloat3( &channel.TranslationKeys[ 0 ] );
        }
        else
        {
            unsigned int curr_index = channel.TranslationKeyCount - 1;
            for ( unsigned int i = 0; i < channel.TranslationKeyCount - 1; ++i )
            {
                if ( animation_time <= channel.TranslationKeyTimestamps[ i + 1 ] )
                {
                    curr_index = i;
                    break;
                }
            }
            unsigned int next_index = std::min( curr_index + 1, channel.TranslationKeyCount - 1 );

            double curr_time = channel.TranslationKeyTimestamps[ curr_index ];
            double next_time = channel.TranslationKeyTimestamps[ next_index ];
            float t = static_cast< float >( ( animation_time - curr_time ) / ( next_time - curr_time ) );

            DirectX::XMVECTOR curr_translation = DirectX::XMLoadFloat3( &channel.TranslationKeys[ curr_index ] );
            DirectX::XMVECTOR next_translation = DirectX::XMLoadFloat3( &channel.TranslationKeys[ next_index ] );
            translation = DirectX::XMVectorLerp( curr_translation, next_translation, t );
        }

        DirectX::XMMATRIX scaling_matrix = DirectX::XMMatrixScalingFromVector( scaling );
        DirectX::XMMATRIX rotation_matrix = DirectX::XMMatrixRotationQuaternion( rotation );
        DirectX::XMMATRIX translation_matrix = DirectX::XMMatrixTranslationFromVector( translation );
        local_node_transformation = scaling_matrix * rotation_matrix * translation_matrix;
    }

    DirectX::XMMATRIX global_node_transformation = local_node_transformation * parent_transformation;

    if ( mesh_node.BoneIndex != INVALID_INDEX )
    {
        DirectX::XMMATRIX bone_offset = DirectX::XMLoadFloat4x4( &mesh_node.BoneOffset );
        DirectX::XMStoreFloat4x4( &bone_transformations[ mesh_node.BoneIndex ], bone_offset * global_node_transformation * inverse_root_transformation );
    }

    for ( unsigned int i = 0; i < mesh_node.ChildCount; ++i )
    {
        CalculateBoneTransformations( mesh_node.Children[ i ], animation, animation_index, animation_time, global_node_transformation, inverse_root_transformation, bone_transformations );
    }
}
void CalculateBoneTransformations( CMesh* mesh, unsigned int animation_index, double animation_time, DirectX::XMFLOAT4X4* bone_transformations )
{
    assert( animation_index < mesh->AnimationCount );
    const CMesh::SAnimation& animation = mesh->Animations[ animation_index ];
    animation_time = fmod( animation_time * animation.TicksPerSecond, animation.Duration );
    CalculateBoneTransformations( mesh->Root, animation, animation_index, animation_time, DirectX::XMMatrixIdentity(), DirectX::XMLoadFloat4x4( &mesh->InverseRootTransformation ), bone_transformations );
}