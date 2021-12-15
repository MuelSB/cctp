#pragma once

#include "Scene/SceneBase.h"
#include "Math/Transform.h"

struct InputEvent;

class DemoScene : public SceneBase
{
public:
	DemoScene();
	void Begin() final;
	void Tick(float deltaTime) final;
	void Draw() final;
	void DrawImGui() final;

	Renderer::TopLevelAccelerationStructure* GetTlas() const { return tlAccelStructure.get(); }
	const glm::vec3& GetProbePosition() const { return ProbeTransform.Position; }
	glm::vec3& GetProbePosition() { return ProbeTransform.Position; }
	const glm::vec4* GetMaterials() const { return MeshColors.data(); }

private:
	void OnInputEvent(InputEvent&& event);
	void PollInputs(float deltaTime);

private:
	static constexpr size_t SceneMeshTransformCount = SceneBase::MaxMaterialCount; // Set to MaxMaterialCount temporarily while a material is a seperate color per mesh
																				   // This will be a material struct in the future with a seperate count to number of meshes in the scene
	static constexpr float CameraYawSensitivity = 0.075f;
	static constexpr float CameraPitchSensitivity = 0.075f;
	static constexpr float CameraPitchMin = -90.0f;
	static constexpr float CameraPitchMax = 90.0f;
	static constexpr float CameraFlySpeed = 0.0075f;
	static constexpr glm::vec3 CameraStartPosition = glm::vec3(0.0f, 2.0f, -10.0f);
	static constexpr glm::vec3 SceneForwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
	static constexpr glm::vec3 SceneRightVector = glm::vec3(1.0f, 0.0f, 0.0f);
	static constexpr glm::vec3 SceneUpVector = glm::vec3(0.0f, 1.0f, 0.0f);

	std::vector<std::unique_ptr<Renderer::Mesh>> Meshes;
	std::vector<std::unique_ptr<Renderer::BottomLevelAccelerationStructure>> blAccelStructures;
	std::unique_ptr<Renderer::TopLevelAccelerationStructure> tlAccelStructure;
	std::vector<Transform> MeshTransforms;
	std::vector<glm::vec4> MeshColors;

	Transform ProbeTransform = {};
};