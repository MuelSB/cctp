#include "Pch.h"
#include "ProbeVolume.h"

Renderer::ProbeVolume::ProbeVolume(const glm::vec3& position, const glm::vec3& volumeExtents, float probeSpacing, float debugProbeSize)
	: Position(position), Extents(volumeExtents), ProbeSpacing(probeSpacing)
{
	// Calculate the number of probes
	ProbeCountX = static_cast<size_t>(volumeExtents.x / probeSpacing);
	ProbeCountY = static_cast<size_t>(volumeExtents.y / probeSpacing);
	ProbeCountZ = static_cast<size_t>(volumeExtents.z / probeSpacing);
	auto probeCountTotal = ProbeCountX * ProbeCountY * ProbeCountZ;

	// Initialize probe transforms
	ProbeTransforms.resize(probeCountTotal, { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {debugProbeSize, debugProbeSize, debugProbeSize} });
	UpdateProbePositions();
}

void Renderer::ProbeVolume::Update()
{
	if (UpdatedPosition != Position)
	{
		UpdateProbePositions();
	}
}

void Renderer::ProbeVolume::UpdateProbePositions()
{
	size_t index = 0;
	for (auto x = 0; x < ProbeCountX; ++x)
	{
		for (auto y = 0; y < ProbeCountY; ++y)
		{
			for (auto z = 0; z < ProbeCountZ; ++z)
			{
				ProbeTransforms[index].Position = Position + glm::vec3((x * ProbeSpacing) - (Extents.x / 2.0f),
																	   (y * ProbeSpacing) - (Extents.y / 2.0f),
																	   (z * ProbeSpacing) - (Extents.z / 2.0f));
				++index;
			}
		}
	}
}
