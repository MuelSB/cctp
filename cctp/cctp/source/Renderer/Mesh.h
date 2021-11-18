#pragma once

#include "Vertices/Vertex1Pos1UV1Norm.h"

namespace Renderer
{
	class Mesh
	{
	public:
		Mesh(ID3D12Device* pDevice, const std::vector<Vertex1Pos1UV1Norm>& vertices, 
			const std::vector<uint32_t> indices, const std::wstring& name);
		size_t GetRequiredBufferWidthVertexBuffer() const { return sizeof(Vertex1Pos1UV1Norm) * Vertices.size(); }
		size_t GetRequiredBufferWidthIndexBuffer() const { return sizeof(uint32_t) * Indices.size(); }
		const Vertex1Pos1UV1Norm* GetVerticesData() const { return Vertices.data(); }
		const uint32_t* GetIndicesData() const { return Indices.data(); }
		ID3D12Resource* GetVertexBuffer() const { return VertexBuffer.Get(); }
		ID3D12Resource* GetIndexBuffer() const { return IndexBuffer.Get(); }

	private:
		std::vector<Vertex1Pos1UV1Norm> Vertices;
		std::vector<uint32_t> Indices;
		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;
		D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
	};
}