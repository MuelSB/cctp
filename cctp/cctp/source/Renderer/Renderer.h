#pragma once

#include "Renderer/SwapChain.h"

namespace Renderer
{
	constexpr float CLEAR_COLOR[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	constexpr UINT64 CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES = 256;

	bool Init();
	bool Shutdown();
	bool Flush();
	bool CreateSwapChain(HWND windowHandle, UINT width, UINT height, std::unique_ptr<SwapChain>& swapChain);

	UINT GetRTVDescriptorIncrementSize();
	bool GetVSyncEnabled();
	void SetVSyncEnabled(const bool enabled);

	// Commands
	bool StartFrame(SwapChain* pSwapChain, size_t& frameIndex);
	bool EndFrame(SwapChain* pSwapChain, size_t frameIndex);
	void ClearFrame(SwapChain* pSwapChain, size_t frameIndex);
}