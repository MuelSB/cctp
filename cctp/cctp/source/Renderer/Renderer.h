#pragma once

#include "Renderer/SwapChain.h"
#include "Renderer/Pipeline/GraphicsPipeline.h"

#include "Geometry.h"
#include "Mesh.h"
#include "Camera.h"
#include "BottomLevelAccelerationStructure.h"
#include "TopLevelAccelerationStructure.h"
#include "DescriptorHeap.h"

struct Transform;

namespace Renderer
{
	bool Init(const uint32_t shaderVisibleCBVSRVUAVDescriptorCount);
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
	// Last descriptor index is occupied by ImGui resources
	void AddSRVDescriptorToShaderVisibleHeap(ID3D12Resource* pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const uint32_t descriptorIndex);
	// Last descriptor index is occupied by ImGui resources
	void AddUAVDescriptorToShaderVisibleHeap(ID3D12Resource* pResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, const uint32_t descriptorIndex);

	UINT GetRTDescriptorIncrementSize();
	bool GetVSyncEnabled();
	void SetVSyncEnabled(const bool enabled);
	const DescriptorHeap* GetShaderVisibleDescriptorHeap();
	D3D12_GPU_VIRTUAL_ADDRESS GetPerFrameConstantBufferGPUVirtualAddress();
	D3D12_GPU_VIRTUAL_ADDRESS GetMaterialConstantBufferGPUVirtualAddress();

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
		void UpdatePerFrameAndMaterialConstants(SwapChain* pSwapChain, UINT perFrameConstantsParameterIndex, const Camera& camera, const glm::vec3& probePosition, const glm::vec4* pMaterials);
		void SubmitMesh(UINT perObjectConstantsParameterIndex, const Mesh& mesh, const Transform& transform, const glm::vec4& color);
		void SetDescriptorHeaps();
		void BeginImGui();
		void EndImGui();
		void RebuildTlas(TopLevelAccelerationStructure* tlas);
		void Raytrace(const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc, ID3D12StateObject* pPipelineStateObject, ID3D12Resource* pRaytraceOutputResource);

		// Copies the src resource to the current frame's swap chain backbuffer. Swap chain render target resource is returned to render target
		// state after copy. Src resource is returned to srcResourceState after copy
		void DebugCopyResourceToRenderTarget(SwapChain* pSwapChain, ID3D12Resource* pSrcResource, D3D12_RESOURCE_STATES srcResourceState);
	}
}