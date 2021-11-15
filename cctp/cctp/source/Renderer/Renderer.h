#pragma once

#include "Renderer/SwapChain.h"

namespace Renderer
{
	constexpr float CLEAR_COLOR[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
	constexpr UINT64 CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES = 256;

	bool Init();
	bool Shutdown();
	bool Flush();
	bool CreateSwapChain(HWND windowHandle, UINT width, UINT height, std::unique_ptr<SwapChain>& swapChain);
	bool ResizeSwapChain(SwapChain* pSwapChain, UINT newWidth, UINT newHeight);

	UINT GetRTDescriptorIncrementSize();
	bool GetVSyncEnabled();
	void SetVSyncEnabled(const bool enabled);

	// Commands
	namespace Commands
	{
		bool StartFrame(SwapChain* pSwapChain, size_t& frameIndex);
		bool EndFrame(SwapChain* pSwapChain, size_t frameIndex);
		void ClearRenderTargets(SwapChain* pSwapChain, size_t frameIndex);
		void SetRenderTargets(SwapChain* pSwapChain, size_t frameIndex);
	}
}