#pragma once

#include "GraphicsPipelineBase.h"

namespace Renderer
{
	class GraphicsPipeline : public Renderer::GraphicsPipelineBase
	{
	public:
		bool Init(ID3D12Device* pDevice) final;

	private:

	};
}