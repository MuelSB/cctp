#pragma once

namespace Renderer
{
    class SwapChain
    {
    public:
        bool Init(Microsoft::WRL::ComPtr<IDXGIFactory4> factory, Microsoft::WRL::ComPtr<ID3D12CommandQueue> directCommandQueue,
            Microsoft::WRL::ComPtr<ID3D12Device> device, HWND hWnd, UINT width, UINT height, size_t backBufferCount, bool tearingSupported, UINT rtvDescriptorIncrementSize);
        bool Present(const bool vsync);
        float GetViewportWidth() const;
        float GetViewportHeight() const;
        bool Resize(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT width, UINT height, UINT rtvDescriptorSize);
        UINT GetCurrentBackBufferIndex() const;
        const Microsoft::WRL::ComPtr<ID3D12Resource>* GetBackBuffers() const;
        ID3D12DescriptorHeap* GetRTDescriptorHeap() const;
        const D3D12_VIEWPORT& GetViewport() const;
        const D3D12_RECT& GetScissorRect() const;
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTDescriptorHandleForFrame(size_t frameIndex) const;
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDSDescriptorHandle() const;

    private:
        bool UpdateBackBuffers(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT rtvDescriptorSize);

    private:
        Microsoft::WRL::ComPtr<IDXGISwapChain3> SwapChain3;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> BackBuffers;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RTDescriptorHeap;
        bool TearingSupported = false;
        D3D12_VIEWPORT Viewport = {};
        D3D12_RECT ScissorRect = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> DepthStencilBuffer;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DSDescriptorHeap;
    };
}
