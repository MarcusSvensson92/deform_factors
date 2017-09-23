#include "WindowContext.h"
#include "RenderContext.h"
#include "Mesh.h"
#include "PackFunctions.h"
#include "SimpleTweakbar.h"

int WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
    CWindowContext* wc = CreateWindowContext();
    CRenderContext* rc = CreateRenderContext( wc->Hwnd );

    ID3D12RootSignature* root_signature = {};
    ID3D12PipelineState* pipeline_states[ 6 ] = {};
    {
        D3D12_ROOT_PARAMETER root_parameter = {};
        root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        root_parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        root_parameter.Descriptor.RegisterSpace = 0;
        root_parameter.Descriptor.ShaderRegister = 0;
        D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
        root_signature_desc.NumParameters = 1;
        root_signature_desc.pParameters = &root_parameter;
        root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        root_signature = CreateRootSignature( rc, root_signature_desc );
        
        D3D12_INPUT_ELEMENT_DESC input_element_descs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_UNORM, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R8G8B8A8_UNORM,     1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R8G8B8A8_UINT,      2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 2, DXGI_FORMAT_R8G8B8A8_UNORM,     3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 3, DXGI_FORMAT_R8G8B8A8_UNORM,     4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 4, DXGI_FORMAT_R8G8B8A8_UNORM,     5, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,    6, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 6, DXGI_FORMAT_R32G32B32_FLOAT,    7, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        ID3DBlob* vertex_shader = LoadShader( L"assets/Shader.hlsl", "VSMain", "vs_5_0", nullptr );
        ID3DBlob* pixel_shaders[ 6 ];
        for ( unsigned int i = 0; i < 6; ++i )
        {
            static const D3D_SHADER_MACRO shader_defines[ 6 ][ 2 ] =
            {
                { { "PS_OUT", "0" }, { 0, 0 } },
                { { "PS_OUT", "1" }, { 0, 0 } },
                { { "PS_OUT", "2" }, { 0, 0 } },
                { { "PS_OUT", "3" }, { 0, 0 } },
                { { "PS_OUT", "4" }, { 0, 0 } },
                { { "PS_OUT", "5" }, { 0, 0 } },
            };
            pixel_shaders[ i ] = LoadShader( L"assets/Shader.hlsl", "PSMain", "ps_5_0", shader_defines[ i ] );
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc = {};
        pipeline_state_desc.InputLayout = { input_element_descs, _countof( input_element_descs ) };
        pipeline_state_desc.pRootSignature = root_signature;
        pipeline_state_desc.VS = { vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize() };
        pipeline_state_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pipeline_state_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        pipeline_state_desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        pipeline_state_desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        pipeline_state_desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        pipeline_state_desc.RasterizerState.DepthClipEnable = TRUE;
        pipeline_state_desc.RasterizerState.FrontCounterClockwise = TRUE;
        pipeline_state_desc.DepthStencilState.DepthEnable = TRUE;
        pipeline_state_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pipeline_state_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        pipeline_state_desc.BlendState.RenderTarget[ 0 ].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pipeline_state_desc.SampleMask = UINT_MAX;
        pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipeline_state_desc.NumRenderTargets = 1;
        pipeline_state_desc.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        pipeline_state_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pipeline_state_desc.SampleDesc.Count = MULTI_SAMPLE_COUNT;

        for ( unsigned int i = 0; i < _countof( pipeline_states ); ++i )
        {
            pipeline_state_desc.PS = { pixel_shaders[ i ]->GetBufferPointer(), pixel_shaders[ i ]->GetBufferSize() };
            VALIDATE( rc->Device->CreateGraphicsPipelineState( &pipeline_state_desc, IID_PPV_ARGS( &pipeline_states[ i ] ) ) );
        }

        vertex_shader->Release();
        for ( unsigned int i = 0; i < _countof( pipeline_states ); ++i )
        {
            pixel_shaders[ i ]->Release();
        }
    }

    struct SConstants
    {
        float               PositionScale;
        float               DeformFactorsTangentScale;
        float               DeformFactorsBitangentScale;
        unsigned int        DiffIntensity;
        DirectX::XMFLOAT4   ViewDirection;
        DirectX::XMFLOAT4X4 ViewProjection;
        DirectX::XMFLOAT4X4 BoneTransformations[ 64 ];
    } constants;
    constants.DiffIntensity = 32;

    ID3D12GraphicsCommandList* command_list = PrepareLoading( rc );

    CMesh* mesh = LoadMesh( "assets/Chal_Head_Wrinkles.fbx" );
    const CMesh::SSubMesh& sub_mesh = mesh->SubMeshes[ 1 ];

    enum EVertexElement
    {
        VERTEX_ELEMENT_POSITION = 0,
        VERTEX_ELEMENT_BONE_WEIGHTS,
        VERTEX_ELEMENT_BONE_INDICES,
        VERTEX_ELEMENT_TANGENTS,
        VERTEX_ELEMENT_DEFORM_FACTORS_TANGENT,
        VERTEX_ELEMENT_DEFORM_FACTORS_BITANGENT, 
        VERTEX_ELEMENT_TANGENT_REF,
        VERTEX_ELEMENT_BITANGENT_REF,
        VERTEX_ELEMENT_COUNT
    };
    const unsigned int VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_COUNT ] =
    {
        4 * sizeof( uint16_t ),
        4 * sizeof( uint8_t ),
        4 * sizeof( uint8_t ),
        4 * sizeof( uint8_t ),
        4 * sizeof( uint8_t ),
        4 * sizeof( uint8_t ),
        3 * sizeof( float ),
        3 * sizeof( float ),
    };

    ID3D12Resource* vertex_buffer = {};
    ID3D12Resource* index_buffer = {};
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_views[ VERTEX_ELEMENT_COUNT ];
    D3D12_INDEX_BUFFER_VIEW index_buffer_view;
    {
        unsigned int vertex_buffer_size = 0;
        for ( unsigned int i = 0; i < VERTEX_ELEMENT_COUNT; ++i )
        {
            vertex_buffer_size += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ i ];
        }
        unsigned int index_buffer_size = sub_mesh.TriangleCount * 3 * sizeof( uint16_t );

        {
            D3D12_HEAP_PROPERTIES heap_properties = {};
            heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heap_properties.CreationNodeMask = 1;
            heap_properties.VisibleNodeMask = 1;
            D3D12_RESOURCE_DESC resource_desc = {};
            resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Width = vertex_buffer_size;
            resource_desc.Height = 1;
            resource_desc.DepthOrArraySize = 1;
            resource_desc.MipLevels = 1;
            resource_desc.SampleDesc.Count = 1;
            resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            VALIDATE( rc->Device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_desc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS( &vertex_buffer ) ) );
        }

        {
            D3D12_HEAP_PROPERTIES heap_properties = {};
            heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
            heap_properties.CreationNodeMask = 1;
            heap_properties.VisibleNodeMask = 1;
            D3D12_RESOURCE_DESC resource_desc = {};
            resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Width = index_buffer_size;
            resource_desc.Height = 1;
            resource_desc.DepthOrArraySize = 1;
            resource_desc.MipLevels = 1;
            resource_desc.SampleDesc.Count = 1;
            resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            VALIDATE( rc->Device->CreateCommittedResource(
                &heap_properties,
                D3D12_HEAP_FLAG_NONE,
                &resource_desc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS( &index_buffer ) ) );
        }

        unsigned int vertex_buffer_offset = 0;
        for ( unsigned int i = 0; i < VERTEX_ELEMENT_COUNT; ++i )
        {
            vertex_buffer_views[ i ].BufferLocation = vertex_buffer->GetGPUVirtualAddress() + vertex_buffer_offset;
            vertex_buffer_views[ i ].StrideInBytes = VERTEX_ELEMENT_STRIDES[ i ];
            vertex_buffer_views[ i ].SizeInBytes = sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ i ];
            vertex_buffer_offset += vertex_buffer_views[ i ].SizeInBytes;
        }

        index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
        index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
        index_buffer_view.SizeInBytes = index_buffer_size;

        UINT upload_buffer_size = vertex_buffer_size + index_buffer_size;
        UINT upload_buffer_offset = AllocateUploadMemory( rc, upload_buffer_size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT );
        BYTE* upload_buffer_data = rc->UploadBufferData + upload_buffer_offset;

        Pack::RGB32FloatToRGBM16Unorm( sub_mesh.VertexCount, reinterpret_cast< float* >( mesh->Positions + sub_mesh.VertexOffset ), reinterpret_cast< uint16_t* >( upload_buffer_data ), &constants.PositionScale );
        upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_POSITION ];
        Pack::RGBA32FloatToRGBA8Unorm( sub_mesh.VertexCount, mesh->BoneWeights + sub_mesh.VertexOffset * BONE_WEIGHTS_PER_VERTEX, reinterpret_cast< uint8_t* >( upload_buffer_data ) );
        upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_BONE_WEIGHTS ];
        Pack::RGBA32UintToRGBA8Uint( sub_mesh.VertexCount, mesh->BoneIndices + sub_mesh.VertexOffset * BONE_WEIGHTS_PER_VERTEX, reinterpret_cast< uint8_t* >( upload_buffer_data ) );
        upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_BONE_INDICES ];
        Pack::TangentsToRGBA8Unorm( sub_mesh.VertexCount, reinterpret_cast< float* >( mesh->Tangents + sub_mesh.VertexOffset ), reinterpret_cast< float* >( mesh->Bitangents + sub_mesh.VertexOffset ), reinterpret_cast< float* >( mesh->Normals + sub_mesh.VertexOffset ), reinterpret_cast< uint8_t* >( upload_buffer_data ) );
        upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_TANGENTS ];
        Pack::RGB32FloatToRGBM8Unorm( sub_mesh.VertexCount, mesh->TangentDeformFactors + sub_mesh.VertexOffset * DEFORM_FACTORS_PER_VERTEX, reinterpret_cast< uint8_t* >( upload_buffer_data ), &constants.DeformFactorsTangentScale );
        upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_DEFORM_FACTORS_TANGENT ];
        Pack::RGB32FloatToRGBM8Unorm( sub_mesh.VertexCount, mesh->BitangentDeformFactors + sub_mesh.VertexOffset * DEFORM_FACTORS_PER_VERTEX, reinterpret_cast< uint8_t* >( upload_buffer_data ), &constants.DeformFactorsBitangentScale );
        upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_DEFORM_FACTORS_BITANGENT ];
        memcpy( upload_buffer_data, mesh->Tangents + sub_mesh.VertexOffset, sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_TANGENT_REF ] );
        upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_TANGENT_REF ];
        memcpy( upload_buffer_data, mesh->Bitangents + sub_mesh.VertexOffset, sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_BITANGENT_REF ] );
        upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_BITANGENT_REF ];
        Pack::RGB32UintToRGB16Uint( sub_mesh.TriangleCount, mesh->Indices + sub_mesh.TriangleOffset * 3, ( uint16_t* )upload_buffer_data );

        command_list->CopyBufferRegion( vertex_buffer, 0, rc->UploadBuffer, 0, vertex_buffer_size );
        command_list->CopyBufferRegion( index_buffer, 0, rc->UploadBuffer, vertex_buffer_size, index_buffer_size );

        D3D12_RESOURCE_BARRIER post_copy_barriers[] =
        {
            { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, vertex_buffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER },
            { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, index_buffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE },
        };
        command_list->ResourceBarrier( _countof( post_copy_barriers ), post_copy_barriers );
    }

    FinishLoading( rc );

    enum EViewOption : unsigned int
    {
        VIEW_OPTION_SMOOTH_OLD,
        VIEW_OPTION_SMOOTH_NEW,
        VIEW_OPTION_SMOOTH_REF,
        VIEW_OPTION_SPLIT_OLD_NEW,
        VIEW_OPTION_SPLIT_OLD_REF,
        VIEW_OPTION_SPLIT_NEW_REF,
        VIEW_OPTION_DIFF_OLD_NEW,
        VIEW_OPTION_DIFF_OLD_REF,
        VIEW_OPTION_DIFF_NEW_REF,
        VIEW_OPTION_COUNT
    };
    unsigned int view_option = VIEW_OPTION_SMOOTH_NEW;
    bool is_paused = false;

    const char* view_options[] =
    {
        "SMOOTH OLD",
        "SMOOTH NEW",
        "SMOOTH REF",
        "SPLIT OLD/NEW",
        "SPLIT OLD/REF",
        "SPLIT NEW/REF",
        "DIFF OLD/NEW",
        "DIFF OLD/REF",
        "DIFF NEW/REF",
    };
    SimpleComponentDesc component_descs[] =
    {
        SimpleComponentDesc::Label( "OLD = Skinning" ),
        SimpleComponentDesc::Label( "NEW = Skinning + Deform Factors" ),
        SimpleComponentDesc::Label( "REF = Reference" ),
        SimpleComponentDesc::Dropdown( "View", _countof( view_options ), view_options, &view_option ),
        SimpleComponentDesc::Button( "Play/Pause", []( void* user_data ) { bool& is_paused = *static_cast< bool* >( user_data ); is_paused = !is_paused; }, &is_paused ),
        SimpleComponentDesc::Slider( "Diff Intensity", 0, 100, &constants.DiffIntensity ),
    };
    SimpleTweakbarDesc tweakbar_desc;
    tweakbar_desc.Parent = wc->Hwnd;
    tweakbar_desc.Title = "Options";
    tweakbar_desc.ComponentCount = _countof( component_descs );
    tweakbar_desc.ComponentDescs = component_descs;
    SimpleTweakbar* tweakbar = CreateSimpleTweakbar( tweakbar_desc );

    const float min_radius = max( mesh->BoundingBoxExtent.x, max( mesh->BoundingBoxExtent.y, mesh->BoundingBoxExtent.z ) );
    float radius = min_radius * 2.0f;

    double animation_time = 0.0;
    float theta = DirectX::XM_PI * 0.5f;
    float phi = DirectX::XM_PI * 0.5f;

    LARGE_INTEGER counts_per_second, previous_timestamp;
    QueryPerformanceFrequency( &counts_per_second );
    const double seconds_per_count = 1.0f / static_cast< double >( counts_per_second.QuadPart );
    QueryPerformanceCounter( &previous_timestamp );

    MSG msg = { 0 };
    while ( msg.message != WM_QUIT )
    {
        if ( PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
            LARGE_INTEGER timestamp;
            QueryPerformanceCounter( &timestamp );
            double dt = static_cast< double >( timestamp.QuadPart - previous_timestamp.QuadPart ) * seconds_per_count;
            previous_timestamp = timestamp;

            if ( !is_paused )
            {
                animation_time += dt;
            }

            float dx = static_cast< float >( wc->DeltaMousePosition[ 0 ] );
            float dy = static_cast< float >( wc->DeltaMousePosition[ 1 ] );
            if ( wc->KeysDown[ MK_LBUTTON ] )
            {
                theta += DirectX::XMConvertToRadians( 0.25f * dx );
                phi -= DirectX::XMConvertToRadians( 0.25f * dy );
                phi = min( max( phi, 0.01f ), DirectX::XM_PI - 0.01f );
            }
            else if ( wc->KeysDown[ MK_RBUTTON ] )
            {
                radius += ( dx - dy ) * 0.2f;
                radius = min( max( radius, min_radius), min_radius * 5.0f );
            }

            float x = mesh->BoundingBoxCenter.x + radius * sinf( phi ) * cosf( theta );
            float y = mesh->BoundingBoxCenter.y + radius * cosf( phi );
            float z = mesh->BoundingBoxCenter.z + radius * sinf( phi ) * sinf( theta );
            DirectX::XMVECTOR view_position = DirectX::XMVectorSet( x, y, z, 1.0f );
            DirectX::XMVECTOR target_position = DirectX::XMLoadFloat3( &mesh->BoundingBoxCenter );
            DirectX::XMVECTOR view_direction = DirectX::XMVector3Normalize( DirectX::XMVectorSubtract( target_position, view_position ) );
            DirectX::XMMATRIX view = DirectX::XMMatrixLookAtRH( view_position, target_position, DirectX::XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ) );

            float fov_y = 75.0f * DirectX::XM_PI / 180.0f;
            float aspect = static_cast< float >( WINDOW_WIDTH ) / static_cast< float >( WINDOW_HEIGHT );
            if ( view_option == VIEW_OPTION_SPLIT_OLD_NEW ||
                 view_option == VIEW_OPTION_SPLIT_OLD_REF ||
                 view_option == VIEW_OPTION_SPLIT_NEW_REF )
            {
                aspect *= 0.5f;
            }
            float near_z = 1.0f;
            float far_z = 1000.0f;
            DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovRH( fov_y, aspect, near_z, far_z );

            DirectX::XMStoreFloat4( &constants.ViewDirection, view_direction );
            DirectX::XMStoreFloat4x4( &constants.ViewProjection, view * projection );

            CalculateBoneTransformations( mesh, 0, animation_time, constants.BoneTransformations );
            UpdateNormalsAndTangents( mesh, 1, constants.BoneTransformations );

            command_list = PrepareFrame( rc );

            // Upload reference tangents and bitangents
            {
                D3D12_RESOURCE_BARRIER pre_copy_barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, vertex_buffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST };
                command_list->ResourceBarrier( 1, &pre_copy_barrier );
            
                UINT upload_buffer_size = sub_mesh.VertexCount * ( VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_TANGENT_REF ] + VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_BITANGENT_REF ] );
                UINT upload_buffer_offset = AllocateUploadMemory( rc, upload_buffer_size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT );
                BYTE* upload_buffer_data = rc->UploadBufferData + upload_buffer_offset;
                
                memcpy( upload_buffer_data, mesh->Tangents + sub_mesh.VertexOffset, sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_TANGENT_REF ] );
                upload_buffer_data += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_TANGENT_REF ];
                memcpy( upload_buffer_data, mesh->Bitangents + sub_mesh.VertexOffset, sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ VERTEX_ELEMENT_BITANGENT_REF ] );
            
                unsigned int vertex_buffer_offset = 0;
                for ( unsigned int i = 0; i < VERTEX_ELEMENT_TANGENT_REF; ++i )
                {
                    vertex_buffer_offset += sub_mesh.VertexCount * VERTEX_ELEMENT_STRIDES[ i ];
                }
                command_list->CopyBufferRegion( vertex_buffer, vertex_buffer_offset, rc->UploadBuffer, upload_buffer_offset, upload_buffer_size );
            
                D3D12_RESOURCE_BARRIER post_copy_barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, vertex_buffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER };
                command_list->ResourceBarrier( 1, &post_copy_barrier );
            }

            // Draw
            {
                command_list->SetGraphicsRootSignature( root_signature );

                BYTE* constant_upload_data = AllocateAndSetGraphicsConstantBuffer( rc, sizeof( SConstants ), 0 );
                memcpy( constant_upload_data, &constants, sizeof( SConstants ) );

                command_list->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
                command_list->IASetIndexBuffer( &index_buffer_view );
                command_list->IASetVertexBuffers( 0, VERTEX_ELEMENT_COUNT, &vertex_buffer_views[ VERTEX_ELEMENT_POSITION ] );

                switch ( view_option )
                {
                    case VIEW_OPTION_SMOOTH_OLD:
                    case VIEW_OPTION_SMOOTH_NEW:
                    case VIEW_OPTION_SMOOTH_REF:
                    {
                        D3D12_RECT scissor_rect = { 0, 0, static_cast< LONG >( WINDOW_WIDTH ), static_cast< LONG >( WINDOW_HEIGHT ) };
                        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast< float >( WINDOW_WIDTH ), static_cast< float >( WINDOW_HEIGHT ), 0.0f, 1.0f };
                        command_list->RSSetViewports( 1, &viewport );
                        command_list->RSSetScissorRects( 1, &scissor_rect );

                        switch ( view_option )
                        {
                            case VIEW_OPTION_SMOOTH_OLD:
                                command_list->SetPipelineState( pipeline_states[ 0 ] );
                                break;
                            case VIEW_OPTION_SMOOTH_NEW:
                                command_list->SetPipelineState( pipeline_states[ 1 ] );
                                break;
                            case VIEW_OPTION_SMOOTH_REF:
                                command_list->SetPipelineState( pipeline_states[ 2 ] );
                                break;
                        }

                        command_list->DrawIndexedInstanced( mesh->SubMeshes[ 1 ].TriangleCount * 3, 1, 0, 0, 0 );

                        break;
                    }
                    case VIEW_OPTION_SPLIT_OLD_NEW:
                    case VIEW_OPTION_SPLIT_OLD_REF:
                    case VIEW_OPTION_SPLIT_NEW_REF:
                    {
                        // Left view
                        {
                            D3D12_RECT scissor_rect = { 0, 0, static_cast< LONG >( WINDOW_WIDTH / 2 ), static_cast< LONG >( WINDOW_HEIGHT ) };
                            D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast< float >( WINDOW_WIDTH / 2 ), static_cast< float >( WINDOW_HEIGHT ), 0.0f, 1.0f };
                            command_list->RSSetViewports( 1, &viewport );
                            command_list->RSSetScissorRects( 1, &scissor_rect );

                            switch ( view_option )
                            {
                                case VIEW_OPTION_SPLIT_OLD_NEW:
                                case VIEW_OPTION_SPLIT_OLD_REF:
                                    command_list->SetPipelineState( pipeline_states[ 0 ] );
                                    break;
                                case VIEW_OPTION_SPLIT_NEW_REF:
                                    command_list->SetPipelineState( pipeline_states[ 1 ] );
                                    break;
                            }

                            command_list->DrawIndexedInstanced( mesh->SubMeshes[ 1 ].TriangleCount * 3, 1, 0, 0, 0 );
                        }

                        // Right view
                        {
                            D3D12_RECT scissor_rect = { static_cast< LONG >( WINDOW_WIDTH / 2 ), 0, static_cast< LONG >( WINDOW_WIDTH ), static_cast< LONG >( WINDOW_HEIGHT ) };
                            D3D12_VIEWPORT viewport = { static_cast< float >( WINDOW_WIDTH / 2 ), 0, static_cast< float >( WINDOW_WIDTH / 2 ), static_cast< float >( WINDOW_HEIGHT ), 0.0f, 1.0f };
                            command_list->RSSetViewports( 1, &viewport );
                            command_list->RSSetScissorRects( 1, &scissor_rect );

                            switch ( view_option )
                            {
                                case VIEW_OPTION_SPLIT_OLD_NEW:
                                    command_list->SetPipelineState( pipeline_states[ 1 ] );
                                    break;
                                case VIEW_OPTION_SPLIT_OLD_REF:
                                case VIEW_OPTION_SPLIT_NEW_REF:
                                    command_list->SetPipelineState( pipeline_states[ 2 ] );
                                    break;
                            }

                            command_list->DrawIndexedInstanced( mesh->SubMeshes[ 1 ].TriangleCount * 3, 1, 0, 0, 0 );
                        }

                        break;
                    }
                    case VIEW_OPTION_DIFF_OLD_NEW:
                    case VIEW_OPTION_DIFF_OLD_REF:
                    case VIEW_OPTION_DIFF_NEW_REF:
                    {
                        D3D12_RECT scissor_rect = { 0, 0, static_cast< LONG >( WINDOW_WIDTH ), static_cast< LONG >( WINDOW_HEIGHT ) };
                        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, static_cast< float >( WINDOW_WIDTH ), static_cast< float >( WINDOW_HEIGHT ), 0.0f, 1.0f };
                        command_list->RSSetViewports( 1, &viewport );
                        command_list->RSSetScissorRects( 1, &scissor_rect );

                        switch ( view_option )
                        {
                            case VIEW_OPTION_DIFF_OLD_NEW:
                                command_list->SetPipelineState( pipeline_states[ 3 ] );
                                break;
                            case VIEW_OPTION_DIFF_OLD_REF:
                                command_list->SetPipelineState( pipeline_states[ 4 ] );
                                break;
                            case VIEW_OPTION_DIFF_NEW_REF:
                                command_list->SetPipelineState( pipeline_states[ 5 ] );
                                break;
                        }

                        command_list->DrawIndexedInstanced( mesh->SubMeshes[ 1 ].TriangleCount * 3, 1, 0, 0, 0 );

                        break;
                    }
                }
            }

            PresentFrame( rc );

            ResetInput( wc );
        }
    }

    index_buffer->Release();
    vertex_buffer->Release();

    DestroyMesh( mesh );

    for ( unsigned int i = 0; i < _countof( pipeline_states ); ++i )
    {
        pipeline_states[ i ]->Release();
    }
    root_signature->Release();

    DestroySimpleTweakbar( tweakbar );

    DestroyRenderContext( rc );
    DestroyWindowContext( wc );

    return 0;
}