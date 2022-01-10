#include "Pch.h"
#include "ScreenPassPipeline.h"
#include "Binary/Binary.h"

bool Renderer::ScreenPassPipeline::Init(ID3D12Device* pDevice)
{
    // Create root signature
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

    rootSignatureDesc.Init(0,
        nullptr,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

    ID3DBlob* signature;
    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr)))
    {
        return false;
    }

    if (FAILED(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature))))
    {
        return false;
    }

    // Load vertex and pixel shader bytecode
    BinaryBuffer vertexShaderBinary;
    if (!Binary::ReadBinaryIntoBuffer("Shaders/Binary/ScreenVertexShader.cso", vertexShaderBinary))
    {
        return false;
    }
    D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
    vertexShaderBytecode.pShaderBytecode = vertexShaderBinary.GetBufferPointer();
    vertexShaderBytecode.BytecodeLength = vertexShaderBinary.GetBufferLength();

    BinaryBuffer pixelShaderBinary;
    if (!Binary::ReadBinaryIntoBuffer("Shaders/Binary/ScreenPixelShader.cso", pixelShaderBinary))
    {
        return false;
    }
    D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
    pixelShaderBytecode.pShaderBytecode = pixelShaderBinary.GetBufferPointer();
    pixelShaderBytecode.BytecodeLength = pixelShaderBinary.GetBufferLength();

    // Create input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"LOCAL_SPACE_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(glm::vec3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"VERTEX_NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(glm::vec3) + sizeof(glm::vec2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    inputLayoutDesc.NumElements = _countof(inputLayout);
    inputLayoutDesc.pInputElementDescs = inputLayout;

    // Create pipeline state object
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.pRootSignature = RootSignature.Get();
    psoDesc.VS = vertexShaderBytecode;
    psoDesc.PS = pixelShaderBytecode;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc = { 1, 0 };
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.NumRenderTargets = 1;

    if (FAILED(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PipelineStateObject))))
    {
        return false;
    }

    return true;
}
