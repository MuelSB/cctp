#include "Pch.h"
#include "Mesh.h"

Renderer::Mesh::Mesh(ID3D12Device* pDevice, const std::vector<Vertex1Pos1UV1Norm>& vertices, 
    const std::vector<uint32_t> indices, const std::wstring& name)
	: Vertices(vertices), Indices(indices)
{
    auto CreateDefaultHeap = [](ID3D12Device* pDevice, const size_t bufferWidth, const void* pBufferData,
        Microsoft::WRL::ComPtr<ID3D12Resource>& resource, const std::wstring& name)
    {
        // Create default heap for mesh data on the GPU
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferWidth);

        if (FAILED(pDevice->CreateCommittedResource(&heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&resource)))
            )
        {
            assert(false && "Failed to create default heap for mesh buffer.");
        }

        // Set a debug name for the resource
        if (FAILED(resource->SetName(name.c_str())))
        {
            assert(false && "Failed to set debug name for buffer.");
        }
    };

    auto vertexBufferWidth = sizeof(Vertex1Pos1UV1Norm) * vertices.size();
    auto indexBufferWidth = sizeof(uint32_t) * indices.size();

    CreateDefaultHeap(pDevice, vertexBufferWidth, vertices.data(), VertexBuffer, name);
    CreateDefaultHeap(pDevice, indexBufferWidth, indices.data(), IndexBuffer, name);

    VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
    VertexBufferView.SizeInBytes = static_cast<UINT32>(vertexBufferWidth);
    VertexBufferView.StrideInBytes = sizeof(Vertex1Pos1UV1Norm);

    IndexBufferView.BufferLocation = IndexBuffer->GetGPUVirtualAddress();
    IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    IndexBufferView.SizeInBytes = static_cast<UINT32>(indexBufferWidth);
}
