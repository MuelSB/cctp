#include "Pch.h"
#include "DemoScene.h"
#include "Math/Transform.h"
#include "Math/Math.h"
#include "Input/InputCodes.h"
#include "Events/EventSystem.h"

bool IsInputPressed(InputCode input)
{
	return 0x80000000 & GetAsyncKeyState(input);
}

DemoScene::DemoScene()
{
	// Subscrive input event function
	EventSystem::SubscribeToEvent<InputEvent>([this](InputEvent&& event)
		{
			this->OnInputEvent(std::move(event));
		});

	// Create meshes
	Meshes.resize(1);

	// Cube mesh
	std::vector<Renderer::Vertex1Pos1UV1Norm> cubeVertices;
	std::vector<uint32_t> cubeIndices;
	Renderer::Geometry::GenerateCubeGeometry(cubeVertices, cubeIndices, 1.0f);
	Renderer::CreateStagedMesh(cubeVertices, cubeIndices, L"CubeMesh", Meshes[0]);

	// Load meshes onto GPU
	Renderer::LoadStagedMeshesOntoGPU(Meshes.data(), Meshes.size());

	// Move main camera back
	MainCamera.Position.z = -5.0f;

	// Set main camera fov
	MainCamera.Settings.PerspectiveFOV = 45.0f;
}

void DemoScene::Begin()
{
}

void DemoScene::Tick(float deltaTime)
{
	DeltaTime = deltaTime;
	PollInputs(DeltaTime);
}

void DemoScene::Draw()
{
	Renderer::Commands::SubmitMesh(*Meshes[0].get(), Transform());
}

void DemoScene::OnInputEvent(InputEvent&& event)
{
	if (event.Input == InputCodes::Mouse_X)
	{
		if (IsInputPressed(InputCodes::Right_Mouse_Button))
		{
			MainCamera.Rotation.y += event.Data * DeltaTime * CameraYawSensitivity;
		}
	}
	else if (event.Input == InputCodes::Mouse_Y)
	{
		if (IsInputPressed(InputCodes::Right_Mouse_Button))
		{
			auto newPitch = MainCamera.Rotation.x + event.Data * DeltaTime * CameraPitchSensitivity;
			newPitch = std::clamp(newPitch, CameraPitchMin, CameraPitchMax);
			MainCamera.Rotation.x = newPitch;
		}
	}
}

void DemoScene::PollInputs(float deltaTime)
{
	if (IsInputPressed(InputCodes::W))
	{
		auto cameraForwardVector = glm::normalize(Math::RotateVector(
			MainCamera.Rotation,
			SceneForwardVector));

		MainCamera.Position += cameraForwardVector * deltaTime * CameraFlySpeed;
	}
	if (IsInputPressed(InputCodes::S))
	{
		auto cameraForwardVector = glm::normalize(Math::RotateVector(
			MainCamera.Rotation,
			SceneForwardVector));

		MainCamera.Position -= cameraForwardVector * deltaTime * CameraFlySpeed;
	}
	if (IsInputPressed(InputCodes::D))
	{
		auto cameraRightVector = glm::normalize(Math::RotateVector(
			MainCamera.Rotation,
			SceneRightVector));

		MainCamera.Position += cameraRightVector * deltaTime * CameraFlySpeed;
	}
	if (IsInputPressed(InputCodes::A))
	{
		auto cameraRightVector = glm::normalize(Math::RotateVector(
			MainCamera.Rotation,
			SceneRightVector));

		MainCamera.Position -= cameraRightVector * deltaTime * CameraFlySpeed;
	}
	if (IsInputPressed(InputCodes::E))
	{
		auto cameraUpVector = glm::normalize(Math::RotateVector(
			MainCamera.Rotation,
			SceneUpVector));

		MainCamera.Position += cameraUpVector * deltaTime * CameraFlySpeed;
	}
	if (IsInputPressed(InputCodes::Q))
	{
		auto cameraUpVector = glm::normalize(Math::RotateVector(
			MainCamera.Rotation,
			SceneUpVector));

		MainCamera.Position -= cameraUpVector * deltaTime * CameraFlySpeed;
	}
}
