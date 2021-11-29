#pragma once

#include "Renderer/SwapChain.h"
#include "Renderer/Pipeline/GraphicsPipeline.h"

#include "Geometry.h"
#include "Mesh.h"
#include "Camera.h"
#include "BottomLevelAccelerationStructure.h"
#include "TopLevelAccelerationStructure.h"

struct Transform;

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
	void CreateBottomLevelAccelerationStructure(Mesh& mesh, std::unique_ptr<BottomLevelAccelerationStructure>& blas);
	bool BuildBottomLevelAccelerationStructures(std::unique_ptr<BottomLevelAccelerationStructure>* pStructures, const size_t structureCount);
	void CreateTopLevelAccelerationStructure(std::unique_ptr<TopLevelAccelerationStructure>& tlas, const bool allowUpdate, const uint32_t instanceCount);
	bool BuildTopLevelAccelerationStructures(std::unique_ptr<TopLevelAccelerationStructure>* pStructures, const size_t structureCount);

	UINT GetRTDescriptorIncrementSize();
	bool GetVSyncEnabled();
	void SetVSyncEnabled(const bool enabled);

	// Temporary
	ID3D12Device5* GetDevice();

	// Commands
	namespace Commands
	{
		bool StartFrame(SwapChain* pSwapChain);
		bool EndFrame(SwapChain* pSwapChain);
		void ClearRenderTargets(SwapChain* pSwapChain);
		void SetRenderTargets(SwapChain* pSwapChain);
		void SetPrimitiveTopology();
		void SetViewport(SwapChain* pSwapChain);
		void SetGraphicsPipeline(GraphicsPipelineBase* pPipeline);
		void UpdatePerFrameConstants(SwapChain* pSwapChain, UINT perFrameConstantsParameterIndex, const Camera& camera);
		void SubmitMesh(UINT perObjectConstantsParameterIndex, const Mesh& mesh, const Transform& transform, const glm::vec4& color);
		void SetDescriptorHeaps();
		void BeginImGui();
		void EndImGui();
		void RebuildTlas(TopLevelAccelerationStructure* tlas);
	}
}