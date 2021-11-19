#pragma once

#include "Renderer/SwapChain.h"
#include "Renderer/Pipeline/GraphicsPipeline.h"

#include "Geometry.h"
#include "Mesh.h"

namespace Renderer
{
	constexpr uint32_t SIZE_64KB = 65536;

	constexpr float CLEAR_COLOR[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
	constexpr UINT64 CONSTANT_BUFFER_ALIGNMENT_SIZE_BYTES = 256;

	struct PerObjectConstants
	{
		glm::mat4 WorldMatrix = glm::identity<glm::mat4>();
	};

	struct PerFrameConstants
	{
		glm::mat4 ViewMatrix = glm::identity<glm::mat4>();
		glm::mat4 ProjectionMatrix = glm::identity<glm::mat4>();
	};

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
	}
}