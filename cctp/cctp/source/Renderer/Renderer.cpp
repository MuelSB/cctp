#include "Pch.h"
#include "Renderer.h"

constexpr size_t BACK_BUFFER_COUNT = 3;

Microsoft::WRL::ComPtr<IDXGIFactory4> DXGIFactory;
bool TearingSupported;
Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter;
DXGI_ADAPTER_DESC1 AdapterDesc;
Microsoft::WRL::ComPtr<ID3D12Device> Device;
UINT RTVIncrementSize;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> DirectCommandQueue;
std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, BACK_BUFFER_COUNT> DirectCommandAllocators;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> DirectCommandList;
std::array<Microsoft::WRL::ComPtr<ID3D12Fence>, BACK_BUFFER_COUNT> FrameFences;
std::array<UINT64, BACK_BUFFER_COUNT> FrameFenceValues;
HANDLE MainThreadFenceEvent;
bool VSyncEnabled = true;

bool EnableDebugLayer()
{
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
    {
        return false;
    }
    debugController->EnableDebugLayer();

    // Enable GPU based validation and synchronized command queue validation
    //Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
    //if (FAILED(debugController.As(&debugController1)))
    //{
    //    return false;
    //}
    //debugController1->SetEnableGPUBasedValidation(TRUE);
    //debugController1->SetEnableSynchronizedCommandQueueValidation(FALSE);

    return true;
#endif // _DEBUG
}

bool ReportLiveObjects()
{
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
    if (FAILED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
    {
        return false;
    }
    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    return true;
#endif
}

bool CreateFactory(Microsoft::WRL::ComPtr<IDXGIFactory4>& factory)
{
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

    return SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));
}

bool CheckTearingSupport(Microsoft::WRL::ComPtr<IDXGIFactory4> factory)
{
    BOOL allowTearing = false;

    Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(factory.As(&factory5)))
    {
        if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
        {
            allowTearing = false;
        }
    }

    return allowTearing == TRUE;
}

bool GetAdapter(Microsoft::WRL::ComPtr<IDXGIFactory4> factory, Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter, DXGI_ADAPTER_DESC1& adapterDesc)
{
    // Select a hardware adapter, supporting D3D12 device creation and favouring the adapter with the largest dedicated video memory
    Microsoft::WRL::ComPtr<IDXGIAdapter1> intermediateAdapter;
    SIZE_T maxDedicatedVideoMemory = 0;
    for (UINT i = 0; factory->EnumAdapters1(i, &intermediateAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 intermediateAdapterDesc;
        intermediateAdapter->GetDesc1(&intermediateAdapterDesc);

        if ((intermediateAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
            (SUCCEEDED(D3D12CreateDevice(intermediateAdapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                intermediateAdapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory))
        {
            maxDedicatedVideoMemory = intermediateAdapterDesc.DedicatedVideoMemory;
            if (FAILED(intermediateAdapter.As(&adapter)))
            {
                return false;
            }
            adapterDesc = intermediateAdapterDesc;
        }
    }

    return true;
}

bool CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter, Microsoft::WRL::ComPtr<ID3D12Device>& device)
{
    auto hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

    // Setup debug info if in debug configuration
#if defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(device.As(&infoQueue)))
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        D3D12_MESSAGE_SEVERITY severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        D3D12_MESSAGE_ID denyIDs[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };

        D3D12_INFO_QUEUE_FILTER newFilter = {};
        newFilter.DenyList.NumSeverities = _countof(severities);
        newFilter.DenyList.pSeverityList = severities;
        newFilter.DenyList.NumIDs = _countof(denyIDs);
        newFilter.DenyList.pIDList = denyIDs;
        if (FAILED(infoQueue->PushStorageFilter(&newFilter)))
        {
            return false;
        }
    }
#endif // _DEBUG

    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

bool CreateCommandQueue(const D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    return SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
}

bool CreateCommandAllocator(const D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device> device,
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& commandAllocator)
{
    return SUCCEEDED(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
}

bool CreateCommandList(const D3D12_COMMAND_LIST_TYPE type, Microsoft::WRL::ComPtr<ID3D12Device> device,
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList)
{
    return SUCCEEDED(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
}

bool CreateFence(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT64 initialValue, D3D12_FENCE_FLAGS fenceFlags, Microsoft::WRL::ComPtr<ID3D12Fence>& fence)
{
    return SUCCEEDED(device->CreateFence(initialValue, fenceFlags, IID_PPV_ARGS(&fence)));
}

bool CreateFenceEvent(HANDLE& fenceEvent)
{
    fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return fenceEvent != nullptr;
}

bool WaitForFenceToReachValue(Microsoft::WRL::ComPtr<ID3D12Fence> fence, UINT64 value, HANDLE event, DWORD duration)
{
    if (fence->GetCompletedValue() < value)
    {
        if (FAILED(fence->SetEventOnCompletion(value, event)))
        {
            return false;
        }

        ::WaitForSingleObject(event, duration);
    }

    return true;
}

bool Renderer::Init()
{
    // Enable debug features if in debug configuration
#ifdef _DEBUG
    if (!EnableDebugLayer())
    {
        DEBUG_LOG("ERROR: Failed to enable debug layer.");
        return false;
    }

    if (!ReportLiveObjects())
    {
        DEBUG_LOG("ERROR: ReportLiveObjects() failed.");
        return false;
    }
#endif

    // Create DXGI factory
    if (!CreateFactory(DXGIFactory))
    {
        DEBUG_LOG("ERROR: Failed to create DXGI factory.");
        return false;
    }

    // Check tearing support
    TearingSupported = CheckTearingSupport(DXGIFactory);

    // Get a hardware adapter
    if (!GetAdapter(DXGIFactory, Adapter, AdapterDesc))
    {
        DEBUG_LOG("ERROR: Failed to get a hardware adapter.");
        return false;
    }

    // Create D3D12 device
    if (!CreateDevice(Adapter, Device))
    {
        DEBUG_LOG("ERROR: Failed to create D3D12 device.");
        return false;
    }

    // Get RTV increment size
    RTVIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create a direct command queue
    if (!CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, DirectCommandQueue))
    {
        DEBUG_LOG("ERROR: Failed to create direct command queue.");
        return false;
    }

    // Create direct command allocators
    for (auto& allocator : DirectCommandAllocators)
    {
        if (!CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, allocator))
        {
            DEBUG_LOG("ERROR: Failed to create direct command allocator.");
            return false;
        }
    }

    // Create a direct command list
    if (!CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, DirectCommandAllocators[0], DirectCommandList))
    {
        DEBUG_LOG("ERROR: Failed to create command list");
        return false;
    }

    // Close the command list as it will not be recording now
    if (FAILED(DirectCommandList->Close()))
    {
        DEBUG_LOG("ERROR: Failed to close direct command list.");
        return false;
    }

    // Create frame fences
    for (auto& fence : FrameFences)
    {
        if (!CreateFence(Device, 0, D3D12_FENCE_FLAG_NONE, fence))
        {
            DEBUG_LOG("ERROR: Failed to create fence.");
            return false;
        }
    }

    // Create main thread OS event
    if (!CreateFenceEvent(MainThreadFenceEvent))
    {
        DEBUG_LOG("ERROR: Failed to create OS event handle for main thread fence.");
        return false;
    }

	return true;
}

bool Renderer::Shutdown()
{
    // Wait for all queues to finish executing work
    if (!Flush())
    {
        return false;
    }

    // Close main thread fence event handle
    if (::CloseHandle(MainThreadFenceEvent) == 0)
    {
        return false;
    }

    return true;
}

bool Renderer::Flush()
{
    size_t i = 0;
    for (const auto& fence : FrameFences)
    {
        if (FAILED(DirectCommandQueue->Signal(fence.Get(), ++FrameFenceValues[i])))
        {
            return false;
        }

        if (!WaitForFenceToReachValue(fence, FrameFenceValues[i], MainThreadFenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count())))
        {
            return false;
        }

        ++i;
    }

    return true;
}

bool Renderer::CreateSwapChain(HWND windowHandle, UINT width, UINT height, std::unique_ptr<SwapChain>& swapChain)
{
    auto temp = std::make_unique<SwapChain>();

    if (!temp->Init(DXGIFactory, DirectCommandQueue, Device, windowHandle, width, height, BACK_BUFFER_COUNT,
        TearingSupported, RTVIncrementSize))
    {
        return false;
    }

    swapChain = std::move(temp);

    return true;
}

UINT Renderer::GetRTVDescriptorIncrementSize()
{
    return RTVIncrementSize;
}

bool Renderer::GetVSyncEnabled()
{
    return VSyncEnabled;
}

void Renderer::SetVSyncEnabled(const bool enabled)
{
    VSyncEnabled = enabled;
}

// Commands
bool Renderer::StartFrame(SwapChain* pSwapChain, size_t& frameIndex)
{
    // Get current frame resources
    frameIndex = pSwapChain->GetCurrentBackBufferIndex();
    auto* pCurrentFrameCommandAllocator = DirectCommandAllocators[frameIndex].Get();
    auto& frameFenceValue = FrameFenceValues[frameIndex];

    // Wait for previous frame
    if (!WaitForFenceToReachValue(FrameFences[frameIndex], frameFenceValue, MainThreadFenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count())))
    {
        return false;
    }

    // Increment frame fence value for the next frame
    ++frameFenceValue;

    // Reset command recording objects
    if (FAILED(pCurrentFrameCommandAllocator->Reset()))
    {
        return false;
    }

    if (FAILED(DirectCommandList->Reset(pCurrentFrameCommandAllocator, nullptr)))
    {
        return false;
    }

    // Start recording commands into direct command list
    // Transition current frame render target from present state to render target state
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetBackBuffers()[frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    DirectCommandList->ResourceBarrier(1, &barrier);

    return true;
}

bool Renderer::EndFrame(SwapChain* pSwapChain, size_t frameIndex)
{
    // Transition current frame render target from render target state to present state
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetBackBuffers()[frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    DirectCommandList->ResourceBarrier(1, &barrier);

    // Stop recording commands into direct command lists
    if (FAILED(DirectCommandList->Close()))
    {
        return false;
    }

    // Execute recorded command lists
    ID3D12CommandList* commandListsToExecute[] = { DirectCommandList.Get() };
    DirectCommandQueue->ExecuteCommandLists(_countof(commandListsToExecute), commandListsToExecute);

    return SUCCEEDED(DirectCommandQueue->Signal(FrameFences[frameIndex].Get(), FrameFenceValues[frameIndex]));
}

void Renderer::ClearFrame(SwapChain* pSwapChain, size_t frameIndex)
{
    auto rtvHandle = pSwapChain->GetRTVDescriptorHandleForFrame(frameIndex);
    DirectCommandList->ClearRenderTargetView(rtvHandle, Renderer::CLEAR_COLOR, 0, nullptr);
}
