#include "Pch.h"
#include "RootSignature.h"
#include "SamplerType.h"

RootSignature::~RootSignature()
{
	for (auto rangeArray : DescriptorRanges)
	{
		delete[] rangeArray;
	}
}

void RootSignature::AddRootDescriptorTableParameter(std::initializer_list<D3D12_DESCRIPTOR_RANGE> descriptorRanges,
	D3D12_SHADER_VISIBILITY shaderVisibility)
{
	DescriptorRanges.push_back(new D3D12_DESCRIPTOR_RANGE[descriptorRanges.size()]);
	auto ranges = descriptorRanges.begin();
	for (unsigned int i = 0; i < descriptorRanges.size(); i++)
	{
		DescriptorRanges[DescriptorRanges.size() - 1][i] = ranges[i];
	}

	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable = {};
	descriptorTable.NumDescriptorRanges = static_cast<uint32_t>(descriptorRanges.size());
	descriptorTable.pDescriptorRanges = DescriptorRanges[DescriptorRanges.size() - 1];

	D3D12_ROOT_PARAMETER rootParameter = {};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter.DescriptorTable = descriptorTable;
	rootParameter.ShaderVisibility = shaderVisibility;
	Parameters.push_back(rootParameter);
}

void RootSignature::AddRootDescriptorTableParameter(D3D12_DESCRIPTOR_RANGE* const descriptorRanges, const uint32_t numRanges,
	D3D12_SHADER_VISIBILITY shaderVisibility)
{
	DescriptorRanges.push_back(new D3D12_DESCRIPTOR_RANGE[numRanges]);
	for (unsigned int i = 0; i < numRanges; i++)
	{
		DescriptorRanges[DescriptorRanges.size() - 1][i] = descriptorRanges[i];
	}

	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable = {};
	descriptorTable.NumDescriptorRanges = numRanges;
	descriptorTable.pDescriptorRanges = DescriptorRanges[DescriptorRanges.size() - 1];

	D3D12_ROOT_PARAMETER rootParameter = {};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter.DescriptorTable = descriptorTable;
	rootParameter.ShaderVisibility = shaderVisibility;
	Parameters.push_back(rootParameter);
}

void RootSignature::AddRootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE type, const uint32_t shaderRegister,
	const uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	D3D12_ROOT_DESCRIPTOR rootDescriptor = {};
	rootDescriptor.ShaderRegister = shaderRegister;
	rootDescriptor.RegisterSpace = registerSpace;

	D3D12_ROOT_PARAMETER rootParameter = {};
	rootParameter.ParameterType = type;
	rootParameter.Descriptor = rootDescriptor;
	rootParameter.ShaderVisibility = shaderVisibility;
	Parameters.push_back(rootParameter);
}

void RootSignature::AddStaticSampler(const SamplerType type,
	const uint32_t shaderRegister, const uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	D3D12_STATIC_SAMPLER_DESC desc = {};

	switch (type)
	{
	case SamplerType::PointWrap:
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D12_FLOAT32_MAX;
		break;

	case SamplerType::PointClamp:
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D12_FLOAT32_MAX;
		break;

	case SamplerType::LinearWrap:
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D12_FLOAT32_MAX;
		break;

	case SamplerType::LinearClamp:
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D12_FLOAT32_MAX;
		break;

	case SamplerType::PointBorder:
		desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = D3D12_MAX_MAXANISOTROPY;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		desc.MinLOD = 0.0f;
		desc.MaxLOD = D3D12_FLOAT32_MAX;
		break;
	}

	desc.ShaderRegister = shaderRegister;
	desc.RegisterSpace = registerSpace;
	desc.ShaderVisibility = shaderVisibility;

	StaticSamplers.push_back(desc);
}

void RootSignature::SetFlags(const D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	Flags = flags;
}

void RootSignature::Create(ID3D12Device* const device)
{
	CD3DX12_ROOT_SIGNATURE_DESC desc;
	desc.Init(static_cast<uint32_t>(Parameters.size()), Parameters.data(),
		static_cast<uint32_t>(StaticSamplers.size()), StaticSamplers.data(),
		Flags);

	ID3DBlob* signature;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	assert(SUCCEEDED(hr));
	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
	assert(SUCCEEDED(hr));
}