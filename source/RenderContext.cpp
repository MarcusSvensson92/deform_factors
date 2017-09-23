#include "RenderContext.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include <stdio.h>

void AllocateDescriptors( CRenderContext* rc, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, SDescriptor* descriptors )
{
    assert( rc->DescriptorHeapSizes[ type ] + count <= DESCRIPTOR_HEAP_CAPACITIES[ type ] );

    UINT increment_size = rc->Device->GetDescriptorHandleIncrementSize( type );
    UINT heap_size = rc->DescriptorHeapSizes[ type ];

    rc->DescriptorHeapSizes[ type ] += count;

    for ( UINT i = 0; i < count; ++i )
    {
        descriptors[ i ].CpuHandle.ptr = rc->DescriptorHeaps[ type ]->GetCPUDescriptorHandleForHeapStart().ptr + ( i + heap_size ) * increment_size;
        descriptors[ i ].GpuHandle.ptr = rc->DescriptorHeaps[ type ]->GetGPUDescriptorHandleForHeapStart().ptr + ( i + heap_size ) * increment_size;
    }
}

CRenderContext* CreateRenderContext( HWND hwnd )
{
    CRenderContext* rc = new CRenderContext();

    RECT rect;
    GetClientRect( hwnd, &rect );
    UINT width = rect.right - rect.left;
    UINT height = rect.bottom - rect.top;

#ifdef _DEBUG
    ID3D12Debug* debug_controller = nullptr;
    VALIDATE( D3D12GetDebugInterface( IID_PPV_ARGS( &debug_controller ) ) );
    if ( debug_controller )
    {
        debug_controller->EnableDebugLayer();
        debug_controller->Release();
    }
#endif

    VALIDATE( D3D12CreateDevice( nullptr, MIN_D3D_FEATURE_LEVEL, IID_PPV_ARGS( &rc->Device ) ) );
    VALIDATE( rc->Device->SetStablePowerState( TRUE ) );

    D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
    command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    VALIDATE( rc->Device->CreateCommandQueue( &command_queue_desc, IID_PPV_ARGS( &rc->CommandQueue ) ) );

    IDXGIFactory4* factory = nullptr;
    VALIDATE( CreateDXGIFactory1( IID_PPV_ARGS( &factory ) ) );

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
    swap_chain_desc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swap_chain_desc.BufferDesc.Width = width;
    swap_chain_desc.BufferDesc.Height = height;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = hwnd;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc.Windowed = TRUE;
    VALIDATE( factory->CreateSwapChain( rc->CommandQueue, &swap_chain_desc, &rc->SwapChain ) );

    factory->Release();

    for ( UINT heap_type = 0; heap_type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++heap_type )
    {
        D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{};
        descriptor_heap_desc.Type = static_cast< D3D12_DESCRIPTOR_HEAP_TYPE >( heap_type );
        descriptor_heap_desc.NumDescriptors = DESCRIPTOR_HEAP_CAPACITIES[ heap_type ];
        switch ( heap_type )
        {
            case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
                descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
                break;
            case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
                descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                break;
        }
        VALIDATE( rc->Device->CreateDescriptorHeap( &descriptor_heap_desc, IID_PPV_ARGS( &rc->DescriptorHeaps[ heap_type ] ) ) );

        rc->DescriptorHeapSizes[ heap_type ] = 0;
    }

    for ( UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i )
    {
        rc->Device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &rc->Fences[ i ] ) );
        rc->FenceEvents[ i ] = CreateEvent( nullptr, FALSE, FALSE, nullptr );
        rc->FenceValues[ i ] = 0;

        rc->Device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &rc->CommandAllocators[ i ] ) );
        rc->Device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, rc->CommandAllocators[ i ], nullptr, IID_PPV_ARGS( &rc->CommandLists[ i ] ) );
        rc->CommandLists[ i ]->Close();

        VALIDATE( rc->SwapChain->GetBuffer( i, IID_PPV_ARGS( &rc->BackBuffers[ i ] ) ) );
    }

    {
        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Width = width;
        resource_desc.Height = height;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        resource_desc.SampleDesc.Count = MULTI_SAMPLE_COUNT;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_CLEAR_VALUE clear_value = {};
        clear_value.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        memcpy( clear_value.Color, CLEAR_COLOR, sizeof( float ) * 4 );
        VALIDATE( rc->Device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
            &clear_value,
            IID_PPV_ARGS( &rc->ColorBuffer ) ) );

        AllocateDescriptors( rc, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, &rc->ColorBufferView );

        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
        rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        rc->Device->CreateRenderTargetView( rc->ColorBuffer, &rtv_desc, rc->ColorBufferView.CpuHandle );
    }

    {
        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Width = width;
        resource_desc.Height = height;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.Format = DXGI_FORMAT_D32_FLOAT;
        resource_desc.SampleDesc.Count = MULTI_SAMPLE_COUNT;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE clear_value = {};
        clear_value.Format = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil.Depth = 1.0f;
        VALIDATE( rc->Device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clear_value,
            IID_PPV_ARGS( &rc->DepthBuffer ) ) );

        AllocateDescriptors( rc, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, &rc->DepthBufferView );

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
        dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        rc->Device->CreateDepthStencilView( rc->DepthBuffer, &dsv_desc, rc->DepthBufferView.CpuHandle );
    }

    {
        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap_properties.CreationNodeMask = 1;
        heap_properties.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Width = UPLOAD_BUFFER_SIZE;
        resource_desc.Height = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        VALIDATE( rc->Device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS( &rc->UploadBuffer ) ) );

        VALIDATE( rc->UploadBuffer->Map( 0, nullptr, reinterpret_cast< void** >( &rc->UploadBufferData ) ) );
        ZeroMemory( rc->UploadBufferOffsets, sizeof( UINT ) * SWAP_CHAIN_BUFFER_COUNT );
    }

    rc->CurrentBufferIndex = 0;
    rc->CurrentFenceValue = 1;
    rc->CurrentUploadBufferOffset = 0;
    rc->CurrentUploadBufferLimit = 0;

    return rc;
}

void DestroyRenderContext( CRenderContext* rc )
{
    rc->UploadBuffer->Unmap( 0, nullptr );
    rc->UploadBuffer->Release();

    rc->DepthBuffer->Release();
    rc->ColorBuffer->Release();

    for ( UINT i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i )
    {
        rc->BackBuffers[ i ]->Release();

        rc->CommandLists[ i ]->Release();
        rc->CommandAllocators[ i ]->Release();

        rc->Fences[ i ]->Release();
        CloseHandle( rc->FenceEvents[ i ] );
    }

    for ( UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
    {
        rc->DescriptorHeaps[ i ]->Release();
    }

    rc->SwapChain->Release();
    rc->CommandQueue->Release();
    rc->Device->Release();

    delete rc;
    rc = nullptr;
}

ID3D12RootSignature* CreateRootSignature( CRenderContext* rc, D3D12_ROOT_SIGNATURE_DESC root_signature_desc )
{
    ID3DBlob* signature_blob = nullptr;
    ID3DBlob* error_blob = nullptr;
    if ( FAILED( D3D12SerializeRootSignature( &root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob ) ) )
    {
        if ( error_blob != nullptr )
            OutputDebugString( ( char* )error_blob->GetBufferPointer() );
        assert( 0 );
    }

    ID3D12RootSignature* root_signature = nullptr;
    VALIDATE( rc->Device->CreateRootSignature( 0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(), __uuidof( ID3D12RootSignature ), ( void** )&root_signature ) );

    if ( signature_blob != nullptr )
        signature_blob->Release();
    if ( error_blob != nullptr )
        error_blob->Release();

    return root_signature;
}

ID3DBlob* LoadShader( LPCWSTR filepath, const char* entry_point, const char* target, const D3D_SHADER_MACRO* defines )
{
    ID3DBlob* code = nullptr;
    ID3DBlob* error = nullptr;

    UINT compile_flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
    compile_flags = D3DCOMPILE_DEBUG;
#endif

    HRESULT hr = D3DCompileFromFile( filepath, defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry_point, target, compile_flags, 0, &code, &error );
    if ( FAILED( hr ) )
    {
        char error_string[ 1024 ];
        if ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
            snprintf( error_string, 1024, "File %ws not found.\n", filepath );
        else if ( error != nullptr )
            snprintf( error_string, 1024, "Error compiling file %ws: %s\n", filepath, ( char* )error->GetBufferPointer() );
        else
            snprintf( error_string, 1024, "Error compiling file %ws.\n", filepath );
        OutputDebugString( error_string );
        DebugBreak();
    }

    if ( error != nullptr )
    {
        error->Release();
    }

    return code;
}

ID3D12GraphicsCommandList* PrepareLoading( CRenderContext* rc )
{
    ID3D12CommandAllocator* command_allocator = rc->CommandAllocators[ 0 ];
    ID3D12GraphicsCommandList* command_list = rc->CommandLists[ 0 ];
    VALIDATE( command_allocator->Reset() );
    VALIDATE( command_list->Reset( command_allocator, nullptr ) );

    return command_list;
}

void FinishLoading( CRenderContext* rc )
{
    ID3D12GraphicsCommandList* command_list = rc->CommandLists[ 0 ];

    command_list->Close();
    ID3D12CommandList* command_lists[] = { command_list };
    rc->CommandQueue->ExecuteCommandLists( _countof( command_lists ), command_lists );

    const UINT64 fence_value = rc->CurrentFenceValue;
    VALIDATE( rc->CommandQueue->Signal( rc->Fences[ 0 ], fence_value ) );
    ++rc->CurrentFenceValue;

    rc->Fences[ 0 ]->SetEventOnCompletion( fence_value, rc->FenceEvents[ 0 ] );
    WaitForSingleObject( rc->FenceEvents[ 0 ], INFINITE );

    rc->CurrentUploadBufferOffset = 0;
    rc->CurrentUploadBufferLimit = 0;
}

ID3D12GraphicsCommandList* PrepareFrame( CRenderContext* rc )
{
    if ( rc->Fences[ rc->CurrentBufferIndex ]->GetCompletedValue() < rc->FenceValues[ rc->CurrentBufferIndex ] )
    {
        rc->Fences[ rc->CurrentBufferIndex ]->SetEventOnCompletion( rc->FenceValues[ rc->CurrentBufferIndex ], rc->FenceEvents[ rc->CurrentBufferIndex ] );
        WaitForSingleObject( rc->FenceEvents[ rc->CurrentBufferIndex ], INFINITE );
    }

    ID3D12CommandAllocator* command_allocator = rc->CommandAllocators[ rc->CurrentBufferIndex ];
    ID3D12GraphicsCommandList* command_list = rc->CommandLists[ rc->CurrentBufferIndex ];
    VALIDATE( command_allocator->Reset() );
    VALIDATE( command_list->Reset( command_allocator, nullptr ) );

    D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, rc->ColorBuffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET };
    command_list->ResourceBarrier( 1, &barrier );
    
    ID3D12DescriptorHeap* descriptor_heaps[] =
    {
        rc->DescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ],
        rc->DescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ]
    };
    command_list->SetDescriptorHeaps( _countof( descriptor_heaps ), descriptor_heaps );

    command_list->OMSetRenderTargets( 1, &rc->ColorBufferView.CpuHandle, FALSE, &rc->DepthBufferView.CpuHandle );
    command_list->ClearRenderTargetView( rc->ColorBufferView.CpuHandle, CLEAR_COLOR, 0, nullptr );
    command_list->ClearDepthStencilView( rc->DepthBufferView.CpuHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr );

    return command_list;
}

void PresentFrame( CRenderContext* rc )
{
    ID3D12GraphicsCommandList* command_list = rc->CommandLists[ rc->CurrentBufferIndex ];

    D3D12_RESOURCE_BARRIER pre_resolve_barriers[] =
    {
        { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, rc->ColorBuffer, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE },
        { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, rc->BackBuffers[ rc->CurrentBufferIndex ], D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST },
    };
    command_list->ResourceBarrier( _countof( pre_resolve_barriers ), pre_resolve_barriers );

    command_list->ResolveSubresource( rc->BackBuffers[ rc->CurrentBufferIndex ], 0, rc->ColorBuffer, 0, DXGI_FORMAT_R8G8B8A8_UNORM );

    D3D12_RESOURCE_BARRIER post_resolve_barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, rc->BackBuffers[ rc->CurrentBufferIndex ], D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT };
    command_list->ResourceBarrier( 1, &post_resolve_barrier );

    command_list->Close();
    ID3D12CommandList* command_lists[] = { command_list };
    rc->CommandQueue->ExecuteCommandLists( _countof( command_lists ), command_lists );

    VALIDATE( rc->SwapChain->Present( 1, 0 ) );

    const UINT64 fence_value = rc->CurrentFenceValue;
    VALIDATE( rc->CommandQueue->Signal( rc->Fences[ rc->CurrentBufferIndex ], fence_value ) );
    rc->FenceValues[ rc->CurrentBufferIndex ] = fence_value;
    ++rc->CurrentFenceValue;
    rc->CurrentBufferIndex = ( rc->CurrentBufferIndex + 1 ) % SWAP_CHAIN_BUFFER_COUNT;

    rc->CurrentUploadBufferLimit = rc->UploadBufferOffsets[ rc->CurrentBufferIndex ];
    rc->UploadBufferOffsets[ ( rc->CurrentBufferIndex + SWAP_CHAIN_BUFFER_COUNT - 1 ) % SWAP_CHAIN_BUFFER_COUNT ] = rc->CurrentUploadBufferOffset;
}

UINT AllocateUploadMemory( CRenderContext* rc, UINT size, UINT alignment )
{
    UINT misalignment = alignment == 0 ? 0 : rc->CurrentUploadBufferOffset % alignment;
    UINT offset = misalignment == 0 ? rc->CurrentUploadBufferOffset : rc->CurrentUploadBufferOffset + alignment - misalignment;
    UINT upper_limit = rc->CurrentUploadBufferOffset < rc->CurrentUploadBufferLimit ? rc->CurrentUploadBufferLimit : UPLOAD_BUFFER_SIZE;
    if ( rc->CurrentUploadBufferOffset >= rc->CurrentUploadBufferLimit && offset + size > UPLOAD_BUFFER_SIZE )
    {
        offset = 0;
        upper_limit = rc->CurrentUploadBufferLimit;
    }
    assert( offset + size <= upper_limit );
    rc->CurrentUploadBufferOffset = offset + size;
    return offset;
}
BYTE* AllocateAndSetGraphicsConstantBuffer( CRenderContext* rc, UINT size, UINT root_parameter_index )
{
    UINT offset = AllocateUploadMemory( rc, size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT );
    rc->CommandLists[ rc->CurrentBufferIndex ]->SetGraphicsRootConstantBufferView( root_parameter_index, rc->UploadBuffer->GetGPUVirtualAddress() + offset );
    return rc->UploadBufferData + offset;
}
BYTE* AllocateAndSetComputeConstantBuffer( CRenderContext* rc, UINT size, UINT root_parameter_index )
{
    UINT offset = AllocateUploadMemory( rc, size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT );
    rc->CommandLists[ rc->CurrentBufferIndex ]->SetComputeRootConstantBufferView( root_parameter_index, rc->UploadBuffer->GetGPUVirtualAddress() + offset );
    return rc->UploadBufferData + offset;
}