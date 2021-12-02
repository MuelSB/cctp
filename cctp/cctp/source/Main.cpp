#include "Pch.h"
#include "Window/Window.h"
#include "Events/EventSystem.h"
#include "Renderer/Renderer.h"
#include "Binary/Binary.h"

#include "Scene/Scenes/DemoScene.h"

// Temporary
#include "Renderer/RootSignature.h"

#define ALIGN_TO(size, alignment) (size + (alignment - 1) & ~(alignment-1))

constexpr uint32_t SHADER_VISIBLE_CBV_SRV_UAV_DESCRIPTOR_COUNT = 3;
constexpr uint32_t SCENE_BVH_SRV_DESCRIPTOR_INDEX = 1;
constexpr uint32_t RAYTRACE_OUTPUT_UAV_DESCRIPTOR_INDEX = 2;
constexpr glm::vec2 WINDOW_DIMS = glm::vec2(1920.0f, 1080.0f);

void CreateConsole(const uint32_t maxLines)
{
	CONSOLE_SCREEN_BUFFER_INFO console_info;
	FILE* fp;
	// allocate a console for this app
	AllocConsole();
	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_info);
	console_info.dwSize.Y = maxLines;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), console_info.dwSize);
	// redirect unbuffered STDOUT to the console
	if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
		if (!(freopen_s(&fp, "CONOUT$", "w", stdout) != 0))
			setvbuf(stdout, NULL, _IONBF, 0);
	// redirect unbuffered STDIN to the console
	if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
		if (!(freopen_s(&fp, "CONIN$", "r", stdin) != 0))
			setvbuf(stdin, NULL, _IONBF, 0);
	// redirect unbuffered STDERR to the console
	if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
		if (!(freopen_s(&fp, "CONOUT$", "w", stderr) != 0))
			setvbuf(stderr, NULL, _IONBF, 0);
	// make C++ standard streams point to console as well
	std::ios::sync_with_stdio(true);
	// clear the error state for each of the C++ standard streams
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();
}

void ReleaseConsole()
{
	FILE* fp;
	// Just to be safe, redirect standard IO to NULL before releasing.
	// Redirect STDIN to NULL
	if (!(freopen_s(&fp, "NUL:", "r", stdin) != 0))
		setvbuf(stdin, NULL, _IONBF, 0);
	// Redirect STDOUT to NULL
	if (!(freopen_s(&fp, "NUL:", "w", stdout) != 0))
		setvbuf(stdout, NULL, _IONBF, 0);
	// Redirect STDERR to NULL
	if (!(freopen_s(&fp, "NUL:", "w", stderr) != 0))
		setvbuf(stderr, NULL, _IONBF, 0);
	// Detach from console
	FreeConsole();
}

int WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
#ifdef _DEBUG
	CreateConsole(2048);
#endif

	// Init window
	if (!Window::Init(L"Demo window", WINDOW_DIMS, Window::STYLE_NO_RESIZE))
	{
		assert(false && "Failed to initialize window.");
	}

	Window::Show(SW_MAXIMIZE);

	// Subscribe input event handler
	EventSystem::SubscribeToEvent<InputEvent>([](InputEvent&& event)
		{
			// If escape key pressed
			if (event.Input == InputCodes::Escape &&
				event.Data == 1.0f &&
				!event.RepeatedKey)
			{
				// Close the window
				Window::Close();
			}
		});

	// Init renderer
	if (!Renderer::Init(SHADER_VISIBLE_CBV_SRV_UAV_DESCRIPTOR_COUNT))
	{
		assert(false && "Failed to initialize renderer.");
	}

	Renderer::SetVSyncEnabled(true);

	// Create a swap chain for the window
	std::unique_ptr<Renderer::SwapChain> swapChain;
	RECT clientRect;
	if (!Window::GetClientAreaRect(clientRect))
	{
		assert(false && "Failed to get the window client area rect.");
	}
	if (!Renderer::CreateSwapChain(Window::GetHandle(), clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, swapChain))
	{
		assert(false && "Failed to create a swap chain for the window.");
	}

	// Subscribe window resized event handler
	EventSystem::SubscribeToEvent<WindowResizedEvent>([&swapChain](WindowResizedEvent&& event)
		{
			// Get the resized client area width and height
			RECT clientRect;
			Window::GetClientAreaRect(clientRect);
			auto newWidth = clientRect.right - clientRect.left;
			auto newHeight = clientRect.bottom - clientRect.top;

			// Check the new width and height are greater than 0
			if ((newWidth <= 0) && (newHeight <= 0))
			{
				return;
			}

			// Wait for GPU queues to idle
			Renderer::Flush();

			// Resize the swap chain
			Renderer::ResizeSwapChain(swapChain.get(), static_cast<UINT>(newWidth), static_cast<UINT>(newHeight));
		});

	// Create graphics pipeline
	std::unique_ptr<Renderer::GraphicsPipelineBase> graphicsPipeline;
	if (!Renderer::CreateGraphicsPipeline<Renderer::GraphicsPipeline>(graphicsPipeline))
	{
		assert(false && "Failed to create graphics pipeline.");
	}

	// Create demo scene
	auto demoScene = std::make_unique<DemoScene>();

	// Create raytracing pipeline

	// Create raytracing resources and add descriptors to resources
	// Scene bvh 
	D3D12_SHADER_RESOURCE_VIEW_DESC sceneBVHSRVDesc = {};
	sceneBVHSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	sceneBVHSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	sceneBVHSRVDesc.RaytracingAccelerationStructure.Location = demoScene->GetTlas()->GetTlasResource()->GetGPUVirtualAddress();
	Renderer::AddSRVDescriptorToShaderVisibleHeap(nullptr, sceneBVHSRVDesc, SCENE_BVH_SRV_DESCRIPTOR_INDEX);

	// Raytracing output texture
	Microsoft::WRL::ComPtr<ID3D12Resource> raytraceOutputResource;

	auto raytraceOutputTextureResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT64>(WINDOW_DIMS.x), static_cast<UINT64>(WINDOW_DIMS.y));
	raytraceOutputTextureResourceDesc.MipLevels = 1;
	raytraceOutputTextureResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	auto raytraceOutputTextureHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	Renderer::GetDevice()->CreateCommittedResource(&raytraceOutputTextureHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&raytraceOutputTextureResourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&raytraceOutputResource));

	D3D12_UNORDERED_ACCESS_VIEW_DESC raytraceOutputUAVDesc = {};
	raytraceOutputUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	Renderer::AddUAVDescriptorToShaderVisibleHeap(raytraceOutputResource.Get(), raytraceOutputUAVDesc, RAYTRACE_OUTPUT_UAV_DESCRIPTOR_INDEX);

	// Load compiled raytracing shaders
	BinaryBuffer rayGenBuffer;
	if (!Binary::ReadBinaryIntoBuffer("Shaders/Binary/RayGen.dxil", rayGenBuffer))
	{
		assert(false && "Failed to read compiled ray gen shader.");
	}

	BinaryBuffer closestHitBuffer;
	if (!Binary::ReadBinaryIntoBuffer("Shaders/Binary/ClosestHit.dxil", closestHitBuffer))
	{
		assert(false && "Failed to read compiled closest hit shader.");
	}

	BinaryBuffer missBuffer;
	if (!Binary::ReadBinaryIntoBuffer("Shaders/Binary/Miss.dxil", missBuffer))
	{
		assert(false && "Failed to read compiled miss shader.");
	}

	// Create raytracing pipeline state object
	Microsoft::WRL::ComPtr<ID3D12StateObject> raytracingPipelineStateObject;

	CD3DX12_STATE_OBJECT_DESC rtpsoDesc = {};
	rtpsoDesc.SetStateObjectType(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	// Add ray gen shader
	constexpr LPCWSTR rayGenExportName = L"RayGen";
	CD3DX12_DXIL_LIBRARY_SUBOBJECT rayGenLibSubobject = {};
	auto rayGenBytecode = CD3DX12_SHADER_BYTECODE(rayGenBuffer.GetBufferPointer(), rayGenBuffer.GetBufferLength());
	rayGenLibSubobject.SetDXILLibrary(&rayGenBytecode);
	rayGenLibSubobject.DefineExport(rayGenExportName);
	rayGenLibSubobject.AddToStateObject(rtpsoDesc);

	// Add miss shader
	constexpr LPCWSTR missExportName = L"Miss";
	CD3DX12_DXIL_LIBRARY_SUBOBJECT missLibSubobject = {};
	auto missBytecode = CD3DX12_SHADER_BYTECODE(missBuffer.GetBufferPointer(), missBuffer.GetBufferLength());
	missLibSubobject.SetDXILLibrary(&missBytecode);
	missLibSubobject.DefineExport(missExportName);
	missLibSubobject.AddToStateObject(rtpsoDesc);

	// Add closest hit shader
	constexpr LPCWSTR closestHitExportName = L"ClosestHit";
	CD3DX12_DXIL_LIBRARY_SUBOBJECT closestHitLibSubobject = {};
	auto closestHitBytecode = CD3DX12_SHADER_BYTECODE(closestHitBuffer.GetBufferPointer(), closestHitBuffer.GetBufferLength());
	closestHitLibSubobject.SetDXILLibrary(&closestHitBytecode);
	closestHitLibSubobject.DefineExport(closestHitExportName);
	closestHitLibSubobject.AddToStateObject(rtpsoDesc);

	// Add hit group shader
	constexpr LPCWSTR hitGroupName = L"HitGroup";
	CD3DX12_HIT_GROUP_SUBOBJECT hitGroupSubobject = {};
	hitGroupSubobject.SetIntersectionShaderImport(nullptr);
	hitGroupSubobject.SetAnyHitShaderImport(nullptr);
	hitGroupSubobject.SetClosestHitShaderImport(L"ClosestHit");
	hitGroupSubobject.SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	hitGroupSubobject.SetHitGroupExport(hitGroupName);
	hitGroupSubobject.AddToStateObject(rtpsoDesc);

	// Add shader config subobject
	UINT payloadSize = sizeof(float) * 3;
	UINT attributeSize = sizeof(float) * 2;
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT shaderConfigSubobject = {};
	shaderConfigSubobject.Config(payloadSize, attributeSize);
	shaderConfigSubobject.AddToStateObject(rtpsoDesc);

	// Create ray gen shader local root signature
	RootSignature rayGenRootSignature;

	D3D12_DESCRIPTOR_RANGE rayGenDescriptorRanges[2];

	rayGenDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	rayGenDescriptorRanges[0].NumDescriptors = 1;
	rayGenDescriptorRanges[0].BaseShaderRegister = 0;
	rayGenDescriptorRanges[0].RegisterSpace = 0;
	rayGenDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rayGenDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	rayGenDescriptorRanges[1].NumDescriptors = 1;
	rayGenDescriptorRanges[1].BaseShaderRegister = 0;
	rayGenDescriptorRanges[1].RegisterSpace = 0;
	rayGenDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rayGenRootSignature.AddRootDescriptorTableParameter(rayGenDescriptorRanges, _countof(rayGenDescriptorRanges), D3D12_SHADER_VISIBILITY_ALL);
	rayGenRootSignature.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	rayGenRootSignature.Create(Renderer::GetDevice());

	CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT rayGenRootSignatureSubObject = {};
	rayGenRootSignatureSubObject.SetRootSignature(rayGenRootSignature.GetRootSignature());
	rayGenRootSignatureSubObject.AddToStateObject(rtpsoDesc);

	// Create association sub object for ray gen shader and ray gen root signature
	CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT rayGenAssociationSubObject = {};
	rayGenAssociationSubObject.AddExport(rayGenExportName);
	rayGenAssociationSubObject.SetSubobjectToAssociate(rayGenRootSignatureSubObject);
	rayGenAssociationSubObject.AddToStateObject(rtpsoDesc);

	// Create pipeline global root signature
	RootSignature raytracingGlobalRootSignature;
	raytracingGlobalRootSignature.Create(Renderer::GetDevice());

	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT raytracingGlobalRootSignatureSubObject = {};
	raytracingGlobalRootSignatureSubObject.SetRootSignature(raytracingGlobalRootSignature.GetRootSignature());
	raytracingGlobalRootSignatureSubObject.AddToStateObject(rtpsoDesc);

	// Create raytracing pipeline config subobject
	UINT raytraceRecursionDepth = 1;

	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT raytracingPipelineConfigSubObject = {};
	raytracingPipelineConfigSubObject.Config(raytraceRecursionDepth);
	raytracingPipelineConfigSubObject.AddToStateObject(rtpsoDesc);

	// Create raytracing pipeline state object
	if (FAILED(Renderer::GetDevice()->CreateStateObject(rtpsoDesc, IID_PPV_ARGS(&raytracingPipelineStateObject))))
	{
		assert(false && "Failed to create raytracing pipeline state object.");
	}

	// Create raytracing shader table

	// Calculate shader table size
	constexpr uint32_t shaderRecordCount = 1;
	// Shader identifier size + another 32 byte block for root arguments to meet alignment requirements
	constexpr uint32_t shaderRecordSize = ALIGN_TO(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 1, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	constexpr uint32_t shaderTableSize = shaderRecordCount * shaderRecordSize;

	// Create shader table GPU memory
	Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable;
	auto shaderTableHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto shaderTableResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(shaderTableSize);
	Renderer::GetDevice()->CreateCommittedResource(&shaderTableHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&shaderTableResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&shaderTable)
	);

	// Map shader table memory
	uint8_t* pShaderTableStart;
	if (FAILED(shaderTable->Map(0, nullptr, reinterpret_cast<void**>(&pShaderTableStart))))
	{
		assert(false && "Failed to map shader table GPU memory.");
	}



	// Begin demo scene
	demoScene->Begin();

	// Enter main loop
	bool quit = false;
	auto lastTime = std::chrono::high_resolution_clock::now();
	while (!quit)
	{
		// Calculate frame delta time
		auto currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float, std::milli> frameTime = currentTime - lastTime;
		lastTime = currentTime;

		// Handle OS messages
		if (Window::RunOSMessageLoop())
		{
			quit = true;
			continue;
		}

		// Tick demo scene
		demoScene->Tick(frameTime.count());

		// Start a frame for the swap chain, retrieving the current back buffer index to render to
		auto* pSwapChain = swapChain.get();
		Renderer::Commands::StartFrame(pSwapChain);

		// Set render targets
		Renderer::Commands::SetRenderTargets(pSwapChain);

		// Clear render targets
		Renderer::Commands::ClearRenderTargets(pSwapChain);

		// Set primitive topology
		Renderer::Commands::SetPrimitiveTopology();

		// Set graphics pipeline
		Renderer::Commands::SetGraphicsPipeline(graphicsPipeline.get());

		// Set viewport
		Renderer::Commands::SetViewport(pSwapChain);

		// Set descriptor heaps
		Renderer::Commands::SetDescriptorHeaps();

		// Update per frame constants
		static const auto& camera = demoScene->GetMainCamera();
		Renderer::Commands::UpdatePerFrameConstants(pSwapChain, 1, camera);

		// Submit draw calls
		// Draw scene
		demoScene->Draw();

		// Begin ImGui for the frame
		Renderer::Commands::BeginImGui();

		// Submit ImGui calls
		// Main menu bar
		ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				Window::Close();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();

		// Rebuild acceleration structures
		Renderer::Commands::RebuildTlas(demoScene->GetTlas());

		// Draw scene ImGui
		demoScene->DrawImGui();

		// End ImGui for the frame
		Renderer::Commands::EndImGui();

		// End the frame for the swap chain
		Renderer::Commands::EndFrame(pSwapChain);

		// Present the frame
		if (!swapChain->Present(Renderer::GetVSyncEnabled()))
		{
			assert(false && "ERROR: Failed to present the swap chain.");
		}
	}

	// Shutdown the renderer
	if (!Renderer::Shutdown())
	{
		assert(false && "Failed to shutdown renderer.");
	}

#ifdef _DEBUG
	ReleaseConsole();
#endif

	return 0;
}