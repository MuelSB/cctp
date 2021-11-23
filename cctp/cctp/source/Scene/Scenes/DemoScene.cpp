#include "Pch.h"
#include "DemoScene.h"
#include "Math/Transform.h"
#include "Math/Math.h"
#include "Input/InputCodes.h"
#include "Events/EventSystem.h"
#include "Window/Window.h"

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

	// Setup scene mesh transforms and colors
	MeshTransforms.resize(SceneMeshTransformCount);
	MeshColors.resize(SceneMeshTransformCount);

	// Floor
	MeshTransforms[0].Position = glm::vec3(0.0f, -0.5f, 0.0f);
	MeshTransforms[0].Scale = glm::vec3(5.0f, 0.5f, 5.0f);
	MeshColors[0] = glm::vec4(0.85f, 0.85f, 0.8f, 1.0f);

	// Identity cube
	MeshTransforms[1].Position = glm::vec3(1.0f, 0.25f, -0.5f);
	MeshTransforms[1].Scale = glm::vec3(1.0f, 1.0f, 1.0f);
	MeshColors[1] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Right wall
	MeshTransforms[2].Position = glm::vec3(2.75f, 1.75f, 0.0f);
	MeshTransforms[2].Scale = glm::vec3(0.5f, 5.0f, 5.0f);
	MeshColors[2] = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);

	// Left wall
	MeshTransforms[3].Position = glm::vec3(-2.75f, 1.75f, 0.0f);
	MeshTransforms[3].Scale = glm::vec3(0.5f, 5.0f, 5.0f);
	MeshColors[3] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

	// Back wall
	MeshTransforms[4].Position = glm::vec3(0.0f, 1.75f, 2.75f);
	MeshTransforms[4].Scale = glm::vec3(5.0f, 5.0f, 0.5f);
	MeshColors[4] = glm::vec4(0.85f, 0.85f, 0.8f, 1.0f);

	// Transformed cube
	MeshTransforms[5].Position = glm::vec3(-1.0f, 0.5f, 0.5f);
	MeshTransforms[5].Rotation = glm::vec3(0.0f, 45.0f, 0.0f);
	MeshTransforms[5].Scale = glm::vec3(1.0f, 2.0f, 1.0f);
	MeshColors[5] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// Move main camera back
	MainCamera.Position = CameraStartPosition;

	// Set main camera fov
	MainCamera.Settings.PerspectiveFOV = 45.0f;
}

void DemoScene::Begin()
{
}

void DemoScene::Tick(float deltaTime)
{
	PollInputs(deltaTime);
}

void DemoScene::Draw()
{
	for (size_t i = 0; i < SceneMeshTransformCount; ++i)
	{
		Renderer::Commands::SubmitMesh(0, *Meshes[0].get(), MeshTransforms[i], MeshColors[i]);
	}
}

void DemoScene::DrawImGui()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Exit"))
		{
			Window::Close();
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void DemoScene::OnInputEvent(InputEvent&& event)
{
	if (event.Input == InputCodes::Right_Mouse_Button && event.Data == 1.0f)
	{
		ShowCursor(false);
		Window::CaptureCursor();
	}
	else if (event.Input == InputCodes::Right_Mouse_Button && event.Data == 0.0f)
	{
		Window::ReleaseCursor();

		RECT windowRect;
		Window::GetWindowAreaRect(windowRect);
		SetCursorPos((windowRect.right - windowRect.left) / 2, (windowRect.bottom - windowRect.top) / 2);
	
		ShowCursor(true);
	}
	else if (event.Input == InputCodes::Mouse_X)
	{
		if (IsInputPressed(InputCodes::Right_Mouse_Button))
		{
			MainCamera.Rotation.y += event.Data * CameraYawSensitivity;
		}
	}
	else if (event.Input == InputCodes::Mouse_Y)
	{
		if (IsInputPressed(InputCodes::Right_Mouse_Button))
		{
			auto newPitch = MainCamera.Rotation.x + event.Data * CameraPitchSensitivity;
			newPitch = std::clamp(newPitch, CameraPitchMin, CameraPitchMax);
			MainCamera.Rotation.x = newPitch;
		}
	}
}

void DemoScene::PollInputs(float deltaTime)
{
	if (IsInputPressed(InputCodes::Right_Mouse_Button))
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
			MainCamera.Position += SceneUpVector * deltaTime * CameraFlySpeed;
		}
		if (IsInputPressed(InputCodes::Q))
		{
			MainCamera.Position -= SceneUpVector * deltaTime * CameraFlySpeed;
		}
	}
}
