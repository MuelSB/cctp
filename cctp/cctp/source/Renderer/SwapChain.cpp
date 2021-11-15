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
    if (FAILED(device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&RTVDescriptorHeap))))
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

ID3D12DescriptorHeap* Renderer::SwapChain::GetRTVDescriptorHeap() const
{
    return RTVDescriptorHeap.Get();
}

const D3D12_VIEWPORT& Renderer::SwapChain::GetViewport() const
{
    return Viewport;
}

const D3D12_RECT& Renderer::SwapChain::GetScissorRect() const
{
    return ScissorRect;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Renderer::SwapChain::GetRTVDescriptorHandleForFrame(size_t frameIndex) const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
        static_cast<INT>(frameIndex), Renderer::GetRTVDescriptorIncrementSize());
}

bool Renderer::SwapChain::UpdateBackBuffers(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT rtvDescriptorSize)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
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
