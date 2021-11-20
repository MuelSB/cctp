#pragma once

#include "Scene/SceneBase.h"

struct InputEvent;

class DemoScene : public SceneBase
{
public:
	DemoScene();
	void Begin() final;
	void Tick(float deltaTime) final;
	void Draw() final;

	void OnInputEvent(InputEvent&& event);

private:
	void PollInputs(float deltaTime);

private:
	static constexpr float CameraYawSensitivity = 0.75f;
	static constexpr float CameraPitchSensitivity = 0.75f;
	static constexpr float CameraPitchMin = -90.0f;
	static constexpr float CameraPitchMax = 90.0f;
	static constexpr glm::vec3 SceneForwardVector = glm::vec3(0.0f, 0.0f, 1.0f);
	static constexpr glm::vec3 SceneRightVector = glm::vec3(1.0f, 0.0f, 0.0f);
	static constexpr glm::vec3 SceneUpVector = glm::vec3(0.0f, 1.0f, 0.0f);
	static constexpr float CameraFlySpeed = 0.0075f;

	float DeltaTime = 0;
	std::vector<std::unique_ptr<Renderer::Mesh>> Meshes;
};