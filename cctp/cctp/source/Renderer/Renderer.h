#pragma once

#include "Renderer/SwapChain.h"
#include "Renderer/Pipeline/GraphicsPipeline.h"

#include "Geometry.h"
#include "Mesh.h"
#include "Camera.h"

namespace Renderer
{
	bool Init();
	bool Shutdown();
	bool Flush();
	bool CreateSwapChain(HWND windowHandle, UINT width, UINT height, std::unique_ptr<SwapChain>& swapChain);
	bool ResizeSwapChain(SwapChain* pSwapChain, UINT newWidth, UINT newHeight);
	template<typename T>
	bool CreateGraphicsPipeline(std::unique_ptr<GraphicsPipelineBase>& pipeline);
	void CreateStagedMesh(const std::vector<Vertex1Pos1UV1Norm>& vertices, const std::vector<uint32_t>& indices,
		const std::wstring& name, std::unique_ptr<Mesh>& mesh);
	bool LoadStagedMeshesOntoGPU(std::unique_ptr<Mesh>* pMeshes, const size_t meshCount);

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
		void SetPrimitiveTopology();
		void SetViewport(SwapChain* pSwapChain);
		void SetGraphicsPipeline(GraphicsPipelineBase* pPipeline);
		void UpdatePerFrameConstants(SwapChain* pSwapChain, size_t frameIndex, const Camera& camera);
	}
}