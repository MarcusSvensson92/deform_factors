#pragma once

#include <d3d12.h>
#include <assert.h>

#ifdef _DEBUG
#define VALIDATE( hr ) assert( SUCCEEDED( hr ) )
#else
#define VALIDATE( hr ) hr
#endif

static const UINT              SWAP_CHAIN_BUFFER_COUNT                                            = 3;
static const UINT              MULTI_SAMPLE_COUNT                                                 = 4;
static const D3D_FEATURE_LEVEL MIN_D3D_FEATURE_LEVEL                                              = D3D_FEATURE_LEVEL_11_0;
static const UINT              DESCRIPTOR_HEAP_CAPACITIES[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ] = { 1, 1, 1, 1 };
static const FLOAT             CLEAR_COLOR[ 4 ]                                                   = { 0.8f, 0.8f, 0.8f, 1.0f };
static const UINT              UPLOAD_BUFFER_SIZE                                                 = 4 * 1024 * 1024;

struct SDescriptor
{
    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle;
};

class CRenderContext
{
public:
    ID3D12Device* Device;
    ID3D12CommandQueue* CommandQueue;
    IDXGISwapChain* SwapChain;

    ID3D12DescriptorHeap* DescriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ];
    UINT DescriptorHeapSizes[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ];

    ID3D12Fence* Fences[ SWAP_CHAIN_BUFFER_COUNT ];
    HANDLE FenceEvents[ SWAP_CHAIN_BUFFER_COUNT ];
    UINT64 FenceValues[ SWAP_CHAIN_BUFFER_COUNT ];

    ID3D12CommandAllocator* CommandAllocators[ SWAP_CHAIN_BUFFER_COUNT ];
    ID3D12GraphicsCommandList* CommandLists[ SWAP_CHAIN_BUFFER_COUNT ];

    ID3D12Resource* BackBuffers[ SWAP_CHAIN_BUFFER_COUNT ];
    ID3D12Resource* ColorBuffer;
    ID3D12Resource* DepthBuffer;
    SDescriptor ColorBufferView;
    SDescriptor DepthBufferView;

    ID3D12Resource* UploadBuffer;
    BYTE* UploadBufferData;
    UINT UploadBufferOffsets[ SWAP_CHAIN_BUFFER_COUNT ];

    UINT64 CurrentFenceValue;
    UINT CurrentBufferIndex;
    UINT CurrentUploadBufferOffset;
    UINT CurrentUploadBufferLimit;
};

CRenderContext* CreateRenderContext( HWND hwnd );
void DestroyRenderContext( CRenderContext* rc );

void AllocateDescriptors( CRenderContext* rc, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, SDescriptor* descriptors );

ID3D12RootSignature* CreateRootSignature( CRenderContext* rc, D3D12_ROOT_SIGNATURE_DESC root_signature_desc );

ID3DBlob* LoadShader( LPCWSTR filepath, const char* entry_point, const char* target, const D3D_SHADER_MACRO* defines );

ID3D12GraphicsCommandList* PrepareLoading( CRenderContext* rc );
void FinishLoading( CRenderContext* rc );

ID3D12GraphicsCommandList* PrepareFrame( CRenderContext* rc );
void PresentFrame( CRenderContext* rc );

UINT AllocateUploadMemory( CRenderContext* rc, UINT size, UINT alignment );
BYTE* AllocateAndSetGraphicsConstantBuffer( CRenderContext* rc, UINT size, UINT root_parameter_index );
BYTE* AllocateAndSetComputeConstantBuffer( CRenderContext* rc, UINT size, UINT root_parameter_index );