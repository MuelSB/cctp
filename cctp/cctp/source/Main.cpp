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
constexpr glm::vec2 RAYTRACE_OUTPUT_DIMS = glm::vec2(32.0f, 32.0f);

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

	const auto clientRectWidth = clientRect.right - clientRect.left;
	const auto clientRectHeight = clientRect.bottom - clientRect.top;

	if (!Renderer::CreateSwapChain(Window::GetHandle(), clientRectWidth, clientRectHeight, swapChain))
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

	auto raytraceOutputTextureResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM,
		static_cast<UINT64>(RAYTRACE_OUTPUT_DIMS.x), static_cast<UINT64>(RAYTRACE_OUTPUT_DIMS.y));
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
	constexpr LPCWSTR hitGroupExportName = L"HitGroup";
	CD3DX12_HIT_GROUP_SUBOBJECT hitGroupSubobject = {};
	hitGroupSubobject.SetIntersectionShaderImport(nullptr);
	hitGroupSubobject.SetAnyHitShaderImport(nullptr);
	hitGroupSubobject.SetClosestHitShaderImport(closestHitExportName);
	hitGroupSubobject.SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	hitGroupSubobject.SetHitGroupExport(hitGroupExportName);
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
	rayGenRootSignature.AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
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

	// Create hit group local root signature
	RootSignature hitGroupRootSignature;

	hitGroupRootSignature.AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, 0, 0, D3D12_SHADER_VISIBILITY_ALL);
	hitGroupRootSignature.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	hitGroupRootSignature.Create(Renderer::GetDevice());

	CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT hitGroupRootSignatureSubObject = {};
	hitGroupRootSignatureSubObject.SetRootSignature(hitGroupRootSignature.GetRootSignature());
	hitGroupRootSignatureSubObject.AddToStateObject(rtpsoDesc);

	// Create association sub object for hit group shader and hit group root signature
	CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT hitGroupAssociationSubObject = {};
	hitGroupAssociationSubObject.AddExport(hitGroupExportName);
	hitGroupAssociationSubObject.SetSubobjectToAssociate(hitGroupRootSignatureSubObject);
	hitGroupAssociationSubObject.AddToStateObject(rtpsoDesc);

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
	constexpr uint32_t rayGenShaderRecordSize = ALIGN_TO(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 1, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	constexpr uint32_t missShaderRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	constexpr uint32_t hitGroupShaderRecordSize = ALIGN_TO(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 1, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

	constexpr uint32_t shaderTableSize = rayGenShaderRecordSize + ALIGN_TO(missShaderRecordSize, 64) + ALIGN_TO(hitGroupShaderRecordSize, 64);

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

	// Get raytracing pipeline state object properties to query shader identifiers
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> raytracingPipelineStateObjectProperties;
	if (FAILED(raytracingPipelineStateObject->QueryInterface(IID_PPV_ARGS(&raytracingPipelineStateObjectProperties))))
	{
		assert(false && "Failed to query raytracing pipeline state object properties.");
	}

	// Populate shader table

	// Shader record 0: Ray gen
	// Shader identifier + descriptor table + root descriptor
	memcpy(pShaderTableStart, 
		raytracingPipelineStateObjectProperties->GetShaderIdentifier(rayGenExportName),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	*(uint64_t*)(pShaderTableStart + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = 
		(Renderer::GetShaderVisibleDescriptorHeap()->GetGPUDescriptorHandle(RAYTRACE_OUTPUT_UAV_DESCRIPTOR_INDEX).ptr - 8); // Moving pointer back to start of the descriptor which is 8 bytes
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pShaderTableStart + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8) = Renderer::GetPerFrameConstantBufferGPUVirtualAddress();

	// Shader record 1: Miss
	// Shader identifier
	memcpy(pShaderTableStart + rayGenShaderRecordSize,
		raytracingPipelineStateObjectProperties->GetShaderIdentifier(missExportName),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Shader record 2: Hit group
	// Shader identifier + root descriptor
	memcpy(pShaderTableStart + rayGenShaderRecordSize + (missShaderRecordSize + 32), // Adding 32 bytes of padding to miss shader record for 64 byte table allignment
		raytracingPipelineStateObjectProperties->GetShaderIdentifier(hitGroupExportName),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pShaderTableStart + rayGenShaderRecordSize + (missShaderRecordSize + 32) + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) =
		Renderer::GetMaterialConstantBufferGPUVirtualAddress();

	// Begin demo scene
	demoScene->Begin();

	// Enter main loop
	bool quit = false;
	auto lastFrameTime = std::chrono::high_resolution_clock::now();
	auto lastGIGatherTime = std::chrono::high_resolution_clock::now();
	while (!quit)
	{
		// Calculate frame delta time
		auto currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float, std::milli> frameTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;
		auto frameTimeF = frameTime.count();

		// Handle OS messages
		if (Window::RunOSMessageLoop())
		{
			quit = true;
			continue;
		}

		// Tick demo scene
		demoScene->Tick(frameTimeF);

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
		static const auto& probePosition = demoScene->GetProbePosition();
		Renderer::Commands::UpdatePerFrameConstants(glm::vec2(pSwapChain->GetViewportWidth(), pSwapChain->GetViewportHeight()), 1, camera, probePosition);

		// Update material constants
		static const auto* pMaterials = demoScene->GetMaterialsPtr();
		Renderer::Commands::UpdateMaterialConstants(pMaterials, static_cast<uint32_t>(demoScene->GetMaterialCount()));

		// Submit draw calls
		// Draw scene
		demoScene->Draw();

		// Raytrace global illumination probe field

		// Check raytracing is enabled
		static float GIGatherRateS = 0.1f;
		static bool dispatchRays = true;
		if (dispatchRays)
		{
			// Check if enough time has elapsed since last GI gather
			std::chrono::duration<float, std::milli> GITime = currentTime - lastGIGatherTime;
			if (GITime.count() >= (GIGatherRateS * 1000.0f))
			{
				// Store time that this gather is happening on
				lastGIGatherTime = currentTime;

				// Rebuild acceleration structures
				Renderer::Commands::RebuildTlas(demoScene->GetTlas());

				// Describe dispatch rays
				D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
				dispatchRaysDesc.Width = 1;
				dispatchRaysDesc.Height = 1;
				dispatchRaysDesc.Depth = 1;

				dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = shaderTable->GetGPUVirtualAddress();
				dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = rayGenShaderRecordSize;

				dispatchRaysDesc.MissShaderTable.StartAddress = shaderTable->GetGPUVirtualAddress() + rayGenShaderRecordSize;
				dispatchRaysDesc.MissShaderTable.StrideInBytes = missShaderRecordSize;
				dispatchRaysDesc.MissShaderTable.SizeInBytes = missShaderRecordSize;

				dispatchRaysDesc.HitGroupTable.StartAddress = shaderTable->GetGPUVirtualAddress() + rayGenShaderRecordSize + ALIGN_TO(dispatchRaysDesc.MissShaderTable.SizeInBytes, 64);
				dispatchRaysDesc.HitGroupTable.StrideInBytes = hitGroupShaderRecordSize;
				dispatchRaysDesc.HitGroupTable.SizeInBytes = hitGroupShaderRecordSize;

				// Dispatch rays
				Renderer::Commands::Raytrace(dispatchRaysDesc, raytracingPipelineStateObject.Get(), raytraceOutputResource.Get());
			}
		}

		// Begin ImGui for the frame
		Renderer::Commands::BeginImGui();

		// Submit ImGui calls

		// Performance stats window
		static bool displayPerformanceStatsWindow = false;
		if (displayPerformanceStatsWindow)
		{
			ImGui::SetNextWindowSize(ImVec2(200.0f, 50.0f));
			ImGui::SetNextWindowPos(ImVec2(50.0f, 975.0f));
			ImGui::Begin("Perf stats", NULL,
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				//ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoDecoration);

			ImGui::Text(("Frametime (ms): " + std::to_string(frameTimeF)).c_str()); // Use ImGui::TextColored to change the text color and add contrast

			ImGui::End();
		}

		// Raytrace output texture view
		static bool showRaytraceOutput = true;
		if (showRaytraceOutput)
		{
			ImGui::SetNextWindowSize(ImVec2(512.0f, 512.0f));
			ImGui::Begin("Raytrace output texture", NULL,
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoCollapse
			);
			ImGui::Image((void*)Renderer::GetShaderVisibleDescriptorHeap()->
				GetGPUDescriptorHandle(RAYTRACE_OUTPUT_UAV_DESCRIPTOR_INDEX).ptr, ImVec2(475.0f, 475.0f));
			ImGui::End();
		}

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

		if (ImGui::BeginMenu("Options"))
		{
			ImGui::Text("GI");
			ImGui::InputFloat("Probe update rate (s)", &GIGatherRateS);
			ImGui::Checkbox("Enable raytracing", &dispatchRays);

			ImGui::Text("Debug");
			ImGui::Checkbox("Show raytrace output", &showRaytraceOutput);
			ImGui::DragFloat3("Probe position", &demoScene->GetProbePosition().x, 0.1f);

			ImGui::Text("Stats");
			ImGui::Checkbox("Show performance stats", &displayPerformanceStatsWindow);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

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