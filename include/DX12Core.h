#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct ConstantBuffer
{
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
};

class DX12Core
{
public:
    DX12Core();
    ~DX12Core();

    bool Initialize(HWND hwnd, int width, int height);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void SetViewMatrix(const XMMATRIX& view);
    void SetProjectionMatrix(const XMMATRIX& projection);

    void RenderCube(const XMMATRIX& worldMatrix, const XMFLOAT4& color);

    void WaitForGPU();

private:
    static const UINT FrameCount = 2;

    bool CreateDevice();
    bool CreateCommandQueue();
    bool CreateSwapChain(HWND hwnd, int width, int height);
    bool CreateDescriptorHeaps();
    bool CreateRenderTargetViews();
    bool CreateDepthStencilBuffer(int width, int height);
    bool CreateCommandAllocators();
    bool CreateCommandList();
    bool CreateFence();
    bool CreateRootSignature();
    bool CreatePipelineState();
    bool CreateVertexBuffer();
    bool CreateIndexBuffer();
    bool CreateConstantBuffer();

    void PopulateCommandList();

    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12Resource> m_depthStencil;
    ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;

    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    ComPtr<ID3D12Resource> m_constantBuffer;
    UINT8* m_constantBufferDataBegin;

    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues[FrameCount];
    HANDLE m_fenceEvent;

    UINT m_rtvDescriptorSize;
    UINT m_frameIndex;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;

    int m_width;
    int m_height;
};
