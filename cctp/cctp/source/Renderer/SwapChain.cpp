#include "Pch.h"
#include "SwapChain.h"
#include "Renderer/Renderer.h"

bool Renderer::SwapChain::Init(Microsoft::WRL::ComPtr<IDXGIFactory4> factory, Microsoft::WRL::ComPtr<ID3D12CommandQueue> directCommandQueue,
    Microsoft::WRL::ComPtr<ID3D12Device> device, HWND hWnd, UINT width, UINT height, size_t backBufferCount, bool tearingSupported, UINT rtvDescriptorIncrementSize)
{
    // Describe swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = static_cast<UINT>(backBufferCount);
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    // Create swap chain
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
    if (FAILED(factory->CreateSwapChainForHwnd(directCommandQueue.Get(),
        hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1)))
    {
        return false;
    }

    // Disable Alt+Enter fullscreen toggle feature
    if (FAILED(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)))
    {
        return false;
    }

    if (FAILED(swapChain1.As(&SwapChain3)))
    {
        return false;
    }

    // Create descriptor heap for render target descriptors
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescriptorHeapDesc.NumDescriptors = static_cast<UINT>(backBufferCount);
    rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDescriptorHeapDesc.NodeMask = 0;
    if (FAILED(device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&RTDescriptorHeap))))
    {
        return false;
    }

    // Create back buffer pointers
    BackBuffers.resize(backBufferCount);

    // Update swap chain back buffers
    if (!UpdateBackBuffers(device, rtvDescriptorIncrementSize))
    {
        return false;
    }

    // Create descriptor heap for depth stencil buffer descriptors
    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&DSDescriptorHeap))))
    {
        return false;
    }

    // Create depth stencil buffer
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    auto dsvHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto dsvResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    if (FAILED(device->CreateCommittedResource(&dsvHeapProperties, D3D12_HEAP_FLAG_NONE,
        &dsvResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&DepthStencilBuffer)
    )))
    {
        return false;
    }

    device->CreateDepthStencilView(DepthStencilBuffer.Get(), &depthStencilDesc, DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // Describe viewport and scissor rect
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = static_cast<FLOAT>(width);
    Viewport.Height = static_cast<FLOAT>(height);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;

    ScissorRect.left = 0;
    ScissorRect.top = 0;
    ScissorRect.right = width;
    ScissorRect.bottom = height;

    // Store tearing support flag
    TearingSupported = tearingSupported;

    return true;
}

bool Renderer::SwapChain::Present(const bool vsync)
{
    if (TearingSupported)
    {
        return SUCCEEDED(SwapChain3->Present(0, DXGI_PRESENT_ALLOW_TEARING));
    }
    else
    {
        return SUCCEEDED(SwapChain3->Present(vsync ? 1 : 0, 0));
    }
}

float Renderer::SwapChain::GetViewportWidth() const
{
    return Viewport.Width;
}

float Renderer::SwapChain::GetViewportHeight() const
{
    return Viewport.Height;
}

bool Renderer::SwapChain::Resize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT width, UINT height, UINT rtvDescriptorSize)
{
    // Release back buffer resources
    for (size_t i = 0; i < BackBuffers.size(); ++i)
    {
        BackBuffers[i].Reset();
    }

    // Resize back buffers
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    if (FAILED(SwapChain3->GetDesc(&swapChainDesc)))
    {
        return false;
    }

    if (FAILED(SwapChain3->ResizeBuffers(static_cast<UINT>(BackBuffers.size()), width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags)))
    {
        return false;
    }

    // Update back buffer resources
    if (!UpdateBackBuffers(device, rtvDescriptorSize))
    {
        return false;
    }

    // Update viewport and scissor rect descriptions
    Viewport.TopLeftX = 0.0f;
    Viewport.TopLeftY = 0.0f;
    Viewport.Width = static_cast<FLOAT>(width);
    Viewport.Height = static_cast<FLOAT>(height);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;

    ScissorRect.left = 0;
    ScissorRect.top = 0;
    ScissorRect.right = width;
    ScissorRect.bottom = height;

    return true;
}

UINT Renderer::SwapChain::GetCurrentBackBufferIndex() const
{
    return SwapChain3->GetCurrentBackBufferIndex();
}

const Microsoft::WRL::ComPtr<ID3D12Resource>* Renderer::SwapChain::GetBackBuffers() const
{
    return BackBuffers.data();
}

ID3D12DescriptorHeap* Renderer::SwapChain::GetRTDescriptorHeap() const
{
    return RTDescriptorHeap.Get();
}

const D3D12_VIEWPORT& Renderer::SwapChain::GetViewport() const
{
    return Viewport;
}

const D3D12_RECT& Renderer::SwapChain::GetScissorRect() const
{
    return ScissorRect;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::SwapChain::GetRTDescriptorHandleForFrame(size_t frameIndex) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
        static_cast<INT>(frameIndex), Renderer::GetRTVDescriptorIncrementSize());
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::SwapChain::GetDSDescriptorHandle() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(DSDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

bool Renderer::SwapChain::UpdateBackBuffers(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT rtvDescriptorSize)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(RTDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (uint32_t i = 0; i < BackBuffers.size(); ++i)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
        if (FAILED(SwapChain3->GetBuffer(i, IID_PPV_ARGS(&backBuffer))))
        {
            return false;
        }
        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtv);
        BackBuffers[i] = backBuffer;
        rtv.Offset(1, rtvDescriptorSize);
    }

    return true;
}
