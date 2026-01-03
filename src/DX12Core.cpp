#include "../include/DX12Core.h"
#include <d3dx12.h>
#include <fstream>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Helper function to align values
inline UINT CalculateConstantBufferByteSize(UINT byteSize)
{
    return (byteSize + 255) & ~255;
}

DX12Core::DX12Core()
    : m_frameIndex(0)
    , m_rtvDescriptorSize(0)
    , m_constantBufferDataBegin(nullptr)
    , m_width(0)
    , m_height(0)
{
    m_viewport = {};
    m_scissorRect = {};
    m_fenceEvent = nullptr;
    m_viewMatrix = XMMatrixIdentity();
    m_projectionMatrix = XMMatrixIdentity();

    for (UINT i = 0; i < FrameCount; i++)
    {
        m_fenceValues[i] = 0;
    }
}

DX12Core::~DX12Core()
{
    Shutdown();
}

bool DX12Core::Initialize(HWND hwnd, int width, int height)
{
    m_width = width;
    m_height = height;

    if (!CreateDevice())
        return false;

    if (!CreateCommandQueue())
        return false;

    if (!CreateSwapChain(hwnd, width, height))
        return false;

    if (!CreateDescriptorHeaps())
        return false;

    if (!CreateRenderTargetViews())
        return false;

    if (!CreateDepthStencilBuffer(width, height))
        return false;

    if (!CreateCommandAllocators())
        return false;

    if (!CreateCommandList())
        return false;

    if (!CreateFence())
        return false;

    if (!CreateRootSignature())
        return false;

    if (!CreatePipelineState())
        return false;

    if (!CreateVertexBuffer())
        return false;

    if (!CreateIndexBuffer())
        return false;

    if (!CreateConstantBuffer())
        return false;

    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = width;
    m_scissorRect.bottom = height;

    return true;
}

void DX12Core::Shutdown()
{
    WaitForGPU();

    if (m_constantBuffer)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_constantBufferDataBegin = nullptr;
    }

    CloseHandle(m_fenceEvent);
}

bool DX12Core::CreateDevice()
{
#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif

    HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
    return SUCCEEDED(hr);
}

bool DX12Core::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    HRESULT hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    return SUCCEEDED(hr);
}

bool DX12Core::CreateSwapChain(HWND hwnd, int width, int height)
{
    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr))
        return false;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    hr = factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    );

    if (FAILED(hr))
        return false;

    hr = swapChain.As(&m_swapChain);
    if (FAILED(hr))
        return false;

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    return true;
}

bool DX12Core::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
    if (FAILED(hr))
        return false;

    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
    if (FAILED(hr))
        return false;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap));
    return SUCCEEDED(hr);
}

bool DX12Core::CreateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

    for (UINT i = 0; i < FrameCount; i++)
    {
        HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
        if (FAILED(hr))
            return false;

        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_rtvDescriptorSize);
    }

    return true;
}

bool DX12Core::CreateDepthStencilBuffer(int width, int height)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT,
        width,
        height,
        1,
        0,
        1,
        0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&m_depthStencil)
    );

    if (FAILED(hr))
        return false;

    m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc,
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    return true;
}

bool DX12Core::CreateCommandAllocators()
{
    for (UINT i = 0; i < FrameCount; i++)
    {
        HRESULT hr = m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_commandAllocators[i])
        );

        if (FAILED(hr))
            return false;
    }

    return true;
}

bool DX12Core::CreateCommandList()
{
    HRESULT hr = m_device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocators[m_frameIndex].Get(),
        nullptr,
        IID_PPV_ARGS(&m_commandList)
    );

    if (FAILED(hr))
        return false;

    m_commandList->Close();
    return true;
}

bool DX12Core::CreateFence()
{
    HRESULT hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr))
        return false;

    m_fenceValues[m_frameIndex] = 1;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return false;
    }

    return true;
}

bool DX12Core::CreateRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
    if (FAILED(hr))
        return false;

    hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature));

    return SUCCEEDED(hr);
}

bool DX12Core::CreatePipelineState()
{
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> error;

    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = D3DCompileFromFile(L"shaders/VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0",
        compileFlags, 0, &vertexShader, &error);
    if (FAILED(hr))
        return false;

    hr = D3DCompileFromFile(L"shaders/PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0",
        compileFlags, 0, &pixelShader, &error);
    if (FAILED(hr))
        return false;

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    return SUCCEEDED(hr);
}

bool DX12Core::CreateVertexBuffer()
{
    Vertex vertices[] =
    {
        // Front face
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
        // Back face
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f) },
        // Top face
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
        // Bottom face
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f) },
        // Left face
        { XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
        // Right face
        { XMFLOAT3( 0.5f, -0.5f, -0.5f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f,  0.5f, -0.5f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f,  0.5f,  0.5f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 0.5f, -0.5f,  0.5f), XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f) }
    };

    const UINT vertexBufferSize = sizeof(vertices);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)
    );

    if (FAILED(hr))
        return false;

    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
    if (FAILED(hr))
        return false;

    memcpy(pVertexDataBegin, vertices, sizeof(vertices));
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;

    return true;
}

bool DX12Core::CreateIndexBuffer()
{
    WORD indices[] =
    {
        0, 1, 2,  0, 2, 3,    // Front
        4, 5, 6,  4, 6, 7,    // Back
        8, 9, 10, 8, 10, 11,  // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Left
        20, 21, 22, 20, 22, 23  // Right
    };

    const UINT indexBufferSize = sizeof(indices);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_indexBuffer)
    );

    if (FAILED(hr))
        return false;

    UINT8* pIndexDataBegin;
    CD3DX12_RANGE readRange(0, 0);
    hr = m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
    if (FAILED(hr))
        return false;

    memcpy(pIndexDataBegin, indices, sizeof(indices));
    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = indexBufferSize;

    return true;
}

bool DX12Core::CreateConstantBuffer()
{
    const UINT constantBufferSize = CalculateConstantBufferByteSize(sizeof(ConstantBuffer));

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer)
    );

    if (FAILED(hr))
        return false;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = constantBufferSize;

    m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

    CD3DX12_RANGE readRange(0, 0);
    hr = m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferDataBegin));

    return SUCCEEDED(hr);
}

void DX12Core::BeginFrame()
{
    m_commandAllocators[m_frameIndex]->Reset();
    m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get());

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    m_commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
        m_frameIndex,
        m_rtvDescriptorSize
    );

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    const float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);
}

void DX12Core::EndFrame()
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[m_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    m_commandList->ResourceBarrier(1, &barrier);

    m_commandList->Close();

    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    m_swapChain->Present(1, 0);

    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
    m_commandQueue->Signal(m_fence.Get(), currentFenceValue);

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
    {
        m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void DX12Core::SetViewMatrix(const XMMATRIX& view)
{
    m_viewMatrix = view;
}

void DX12Core::SetProjectionMatrix(const XMMATRIX& projection)
{
    m_projectionMatrix = projection;
}

void DX12Core::RenderCube(const XMMATRIX& worldMatrix, const XMFLOAT4& color)
{
    ConstantBuffer cb;
    cb.world = XMMatrixTranspose(worldMatrix);
    cb.view = XMMatrixTranspose(m_viewMatrix);
    cb.projection = XMMatrixTranspose(m_projectionMatrix);

    memcpy(m_constantBufferDataBegin, &cb, sizeof(cb));

    m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

void DX12Core::WaitForGPU()
{
    m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]);

    m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
    WaitForSingleObject(m_fenceEvent, INFINITE);

    m_fenceValues[m_frameIndex]++;
}
