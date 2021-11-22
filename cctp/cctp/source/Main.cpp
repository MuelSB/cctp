#include "Pch.h"
#include "Window/Window.h"
#include "Events/EventSystem.h"
#include "Renderer/Renderer.h"

#include "Scene/Scenes/DemoScene.h"

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
	if (!Window::Init(L"Demo window", glm::vec2(1920.0f, 1080.0f), Window::STYLE_WINDOWED))
	{
		assert(false && "Failed to initialize window.");
	}

	Window::Show();

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
	if (!Renderer::Init())
	{
		assert(false && "Failed to initialize renderer.");
	}

	Renderer::SetVSyncEnabled(false);

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

		// Update per frame constants
		static const auto& camera = demoScene->GetMainCamera();
		Renderer::Commands::UpdatePerFrameConstants(pSwapChain, 1, camera);

		// Submit draw calls
		// Draw scene
		demoScene->Draw();

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