#include "Pch.h"
#include "Renderer.h"

#include "Pipeline/GraphicsPipeline.h"

constexpr float CLEAR_COLOR[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
constexpr UINT64 CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES = 256;
constexpr uint32_t SIZE_64KB = 65536;
constexpr size_t BACK_BUFFER_COUNT = 3;
constexpr uint32_t MAX_DRAWS_PER_FRAME = 256;

// Rendering objects
Microsoft::WRL::ComPtr<IDXGIFactory4> DXGIFactory;
bool TearingSupported;
Microsoft::WRL::ComPtr<IDXGIAdapter4> Adapter;
DXGI_ADAPTER_DESC1 AdapterDesc;
Microsoft::WRL::ComPtr<ID3D12Device> Device;
UINT RTDescriptorIncrementSize;
Microsoft::WRL::ComPtr<ID3D12CommandQueue> DirectCommandQueue;
std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, BACK_BUFFER_COUNT> DirectCommandAllocators;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> DirectCommandList;
std::array<Microsoft::WRL::ComPtr<ID3D12Fence>, BACK_BUFFER_COUNT> FrameFences;
std::array<UINT64, BACK_BUFFER_COUNT> FrameFenceValues;
HANDLE MainThreadFenceEvent;
bool VSyncEnabled = true;

// Asset loading command objects
Microsoft::WRL::ComPtr<ID3D12CommandQueue> GraphicsLoadCommandQueue;
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> GraphicsLoadCommandAllocator;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GraphicsLoadCommandList;
Microsoft::WRL::ComPtr<ID3D12Fence> GraphicsLoadFence;
UINT64 GraphicsLoadFenceValue = 0;

// Constant buffers
Microsoft::WRL::ComPtr<ID3D12Resource> PerFrameConstantBuffer;
uint8_t* MappedPerFrameConstantBufferLocation;

Microsoft::WRL::ComPtr<ID3D12Resource> PerObjectConstantBuffer;
uint8_t* MappedPerObjectConstantBufferLocation;

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
    RTDescriptorIncrementSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

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

    // Create copy resources
    if (!CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, GraphicsLoadCommandQueue))
    {
        DEBUG_LOG("ERROR: Failed to create graphics load command queue.");
        return false;
    }

    if (!CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, GraphicsLoadCommandAllocator))
    {
        DEBUG_LOG("ERROR: Failed to create graphics load command allocator.");
        return false;
    }

    if (!CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, Device, GraphicsLoadCommandAllocator, GraphicsLoadCommandList))
    {
        DEBUG_LOG("ERROR: Failed to create graphics load command list.");
        return false;
    }

    if (FAILED(GraphicsLoadCommandList->Close()))
    {
        DEBUG_LOG("ERROR: Failed to close graphics load command list.");
        return false;
    }

    if (!CreateFence(Device, GraphicsLoadFenceValue, D3D12_FENCE_FLAG_NONE, GraphicsLoadFence))
    {
        DEBUG_LOG("ERROR: Failed to create graphics load fence.");
        return false;
    }

    // Create per frame constant buffer
    auto perFrameHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto perFrameResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(SIZE_64KB);

    if (FAILED(Device->CreateCommittedResource(&perFrameHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &perFrameResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&PerFrameConstantBuffer))))
    {
        DEBUG_LOG("ERROR: Failed to create per frame constant buffer.");
        return false;
    }

    // Set a debug name for the resource
    if (FAILED(PerFrameConstantBuffer->SetName(L"PerFrameConstantBuffer")))
    {
        DEBUG_LOG("ERROR: Failed to name per frame constant buffer.");
        return false;
    }

    // Map the per frame constant buffer
    D3D12_RANGE perFrameReadRange(0, 0);
    void* mappedPerFrameConstantBufferResource;
    if FAILED(PerFrameConstantBuffer->Map(0, &perFrameReadRange, &mappedPerFrameConstantBufferResource))
    {
        DEBUG_LOG("ERROR: Failed to map per frame constant buffer.");
        return false;
    }
    MappedPerFrameConstantBufferLocation = static_cast<uint8_t*>(mappedPerFrameConstantBufferResource);

    // Create per object constant buffer
    auto perObjectHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto perObjectResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(SIZE_64KB);

    if (FAILED(Device->CreateCommittedResource(&perObjectHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &perObjectResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&PerObjectConstantBuffer))))
    {
        DEBUG_LOG("ERROR: Failed to create per object constant buffer.");
        return false;
    }

    // Set a debug name for the resource
    if (FAILED(PerFrameConstantBuffer->SetName(L"PerObjectConstantBuffer")))
    {
        DEBUG_LOG("ERROR: Failed to name per object constant buffer.");
        return false;
    }

    // Map the per object constant buffer
    D3D12_RANGE perObjectReadRange(0, 0);
    void* mappedPerObjectConstantBufferResource;
    if FAILED(PerObjectConstantBuffer->Map(0, &perObjectReadRange, &mappedPerObjectConstantBufferResource))
    {
        DEBUG_LOG("ERROR: Failed to map per frame constant buffer.");
        return false;
    }
    MappedPerObjectConstantBufferLocation = static_cast<uint8_t*>(mappedPerObjectConstantBufferResource);

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
        TearingSupported, RTDescriptorIncrementSize))
    {
        return false;
    }

    swapChain = std::move(temp);

    return true;
}

bool Renderer::ResizeSwapChain(SwapChain* pSwapChain, UINT newWidth, UINT newHeight)
{
    return pSwapChain->Resize(Device, newWidth, newHeight, RTDescriptorIncrementSize);
}

template<>
bool Renderer::CreateGraphicsPipeline<Renderer::GraphicsPipeline>(std::unique_ptr<Renderer::GraphicsPipelineBase>& pipeline)
{
    auto temp = std::make_unique<Renderer::GraphicsPipeline>();
    if (!temp->Init(Device.Get()))
    {
        return false;
    }
    pipeline = std::move(temp);
    return true;
}

void Renderer::CreateStagedMesh(const std::vector<Vertex1Pos1UV1Norm>& vertices, const std::vector<uint32_t>& indices,
    const std::wstring& name, std::unique_ptr<Mesh>& mesh)
{
    mesh = std::make_unique<Mesh>(Device.Get(), vertices, indices, name);
}

bool Renderer::LoadStagedMeshesOntoGPU(std::unique_ptr<Mesh>* pMeshes, const size_t meshCount)
{
    auto CreateIntermediateUploadBuffer = [](const size_t bufferSize, const void* bufferData, 
        Microsoft::WRL::ComPtr<ID3D12Resource>& intermediateBuffer, const std::wstring& name)
    {
        // Create intermediate vertex upload buffer
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

        if (FAILED(Device->CreateCommittedResource(&heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&intermediateBuffer)))
            )
        {
            assert(false && "Failed to create intermediate mesh buffer.");
        }

        // Set a debug name for the resource
        if (FAILED(intermediateBuffer->SetName(name.c_str())))
        {
            assert(false && "Failed to set name for intermediate mesh buffer.");
        }

        // Map the intermediate buffer resource
        D3D12_RANGE readRange(0, 0);
        void* intermediateResourceStart;
        if (FAILED(intermediateBuffer->Map(0, &readRange, &intermediateResourceStart)))
        {
            assert(false && "Failed to create intermediate mesh buffer.");
        }

        // Copy data into the intermediate upload buffer
        memcpy(intermediateResourceStart, bufferData, bufferSize);

        // Unmap intermediate buffer resource
        intermediateBuffer->Unmap(0, nullptr);
    };

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> intermediateUploadBuffers(meshCount * 2);

    // For each mesh
    for (size_t i = 0, j = 0; i < meshCount; ++i, j += 2)
    {
        // Create and fill intermediate upload buffers with vertex and index data
        // Intermediate buffers are stored next to each other for each mesh
        // [[inter vertex buffer mesh 0][inter index buffer mesh 0][inter vertex buffer mesh 1][inter index buffer mesh 1]...]
        auto* pMesh = pMeshes[i].get();
        auto& intermediateVertexUploadBuffer = intermediateUploadBuffers[j];
        auto& intermediateIndexUploadBuffer = intermediateUploadBuffers[j + 1];

        CreateIntermediateUploadBuffer(pMesh->GetRequiredBufferWidthVertexBuffer(), pMesh->GetVerticesData(), 
            intermediateVertexUploadBuffer, L"VerticesIntermediateUploadBuffer" + std::to_wstring(i));

        CreateIntermediateUploadBuffer(pMesh->GetRequiredBufferWidthIndexBuffer(), pMesh->GetIndicesData(),
            intermediateIndexUploadBuffer, L"IndexIntermediateUploadBuffer" + std::to_wstring(i));
    }

    if (FAILED(GraphicsLoadCommandAllocator->Reset()))
    {
        DEBUG_LOG("Failed to reset copy command allocator.");
        return false;
    }

    if (FAILED(GraphicsLoadCommandList->Reset(GraphicsLoadCommandAllocator.Get(), nullptr)))
    {
        DEBUG_LOG("Failed to reset copy command list.");
        return false;
    }

    std::vector<CD3DX12_RESOURCE_BARRIER> transitionBarriers(meshCount * 2);

    // For each mesh
    for (size_t i = 0, j = 0; i < meshCount; ++i, j += 2)
    {
        auto* pMesh = pMeshes[i].get();
        auto* pMeshVertexBuffer = pMesh->GetVertexBuffer();
        auto* pMeshIndexBuffer = pMesh->GetIndexBuffer();

        GraphicsLoadCommandList->CopyResource(pMeshVertexBuffer, intermediateUploadBuffers[j].Get());
        GraphicsLoadCommandList->CopyResource(pMeshIndexBuffer, intermediateUploadBuffers[j + 1].Get());

        transitionBarriers[j] = CD3DX12_RESOURCE_BARRIER::Transition(pMeshVertexBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        transitionBarriers[j + 1] = CD3DX12_RESOURCE_BARRIER::Transition(pMeshIndexBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }

    GraphicsLoadCommandList->ResourceBarrier(static_cast<UINT>(transitionBarriers.size()), transitionBarriers.data());

    if (FAILED(GraphicsLoadCommandList->Close()))
    {
        return false;
    }

    ID3D12CommandList* commandLists[] = { GraphicsLoadCommandList.Get() };
    GraphicsLoadCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    ++GraphicsLoadFenceValue;
    if (FAILED(GraphicsLoadCommandQueue->Signal(GraphicsLoadFence.Get(), GraphicsLoadFenceValue)))
    {
        return false;
    }

    // Wait on CPU for copy queue to finish
    return WaitForFenceToReachValue(GraphicsLoadFence, GraphicsLoadFenceValue, MainThreadFenceEvent,
        static_cast<DWORD>(std::chrono::milliseconds::max().count()));
}

UINT Renderer::GetRTDescriptorIncrementSize()
{
    return RTDescriptorIncrementSize;
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
bool Renderer::Commands::StartFrame(SwapChain* pSwapChain, size_t& frameIndex)
{
    // Get current frame resources
    frameIndex = pSwapChain->GetCurrentBackBufferIndex();
    auto* pCurrentFrameCommandAllocator = DirectCommandAllocators[frameIndex].Get();
    auto& frameFenceValue = FrameFenceValues[frameIndex];

    // Wait for previous frame
    if (!WaitForFenceToReachValue(FrameFences[frameIndex], frameFenceValue, MainThreadFenceEvent,
        static_cast<DWORD>(std::chrono::milliseconds::max().count())))
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

bool Renderer::Commands::EndFrame(SwapChain* pSwapChain, size_t frameIndex)
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

void Renderer::Commands::ClearRenderTargets(SwapChain* pSwapChain, size_t frameIndex)
{
    auto rtvHandle = pSwapChain->GetRTDescriptorHandleForFrame(frameIndex);
    auto dsvHandle = pSwapChain->GetDSDescriptorHandle();
    DirectCommandList->ClearRenderTargetView(rtvHandle, CLEAR_COLOR, 0, nullptr);
    DirectCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void Renderer::Commands::SetRenderTargets(SwapChain* pSwapChain, size_t frameIndex)
{
    auto rtvHandle = pSwapChain->GetRTDescriptorHandleForFrame(frameIndex);
    auto dsvHandle = pSwapChain->GetDSDescriptorHandle();
    DirectCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void Renderer::Commands::SetPrimitiveTopology()
{
    DirectCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Renderer::Commands::SetViewport(SwapChain* pSwapChain)
{
    DirectCommandList->RSSetViewports(1, &pSwapChain->GetViewport());
    DirectCommandList->RSSetScissorRects(1, &pSwapChain->GetScissorRect());
}

void Renderer::Commands::SetGraphicsPipeline(GraphicsPipelineBase* pPipeline)
{
    DirectCommandList->SetPipelineState(pPipeline->GetPipelineState());
    DirectCommandList->SetGraphicsRootSignature(pPipeline->GetRootSignature());
}
