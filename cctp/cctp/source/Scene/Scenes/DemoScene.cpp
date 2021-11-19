#include "Pch.h"
#include "DemoScene.h"
#include "Math/Transform.h"

DemoScene::DemoScene()
{
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
}

void DemoScene::Begin()
{
}

void DemoScene::Tick(float deltaTime)
{
}

void DemoScene::Draw()
{
	Renderer::Commands::SubmitMesh(*Meshes[0].get(), Transform());
}
