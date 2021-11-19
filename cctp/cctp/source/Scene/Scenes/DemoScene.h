#pragma once

#include "Scene/SceneBase.h"

class DemoScene : public SceneBase
{
public:
	DemoScene();
	void Begin() final;
	void Tick(float deltaTime) final;
	void Draw() final;

private:
	std::vector<std::unique_ptr<Renderer::Mesh>> Meshes;
};