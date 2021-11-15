#pragma once

// Windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

// Glm
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "Math/glm/vec2.hpp"
#include "Math/glm/vec3.hpp"
#include "Math/glm/vec4.hpp"
#include "Math/glm/mat3x3.hpp"
#include "Math/glm/mat4x4.hpp"
#include "Math/glm/ext/matrix_transform.hpp"
#include "Math/glm/ext/matrix_clip_space.hpp"
#include "Math/glm/gtc/quaternion.hpp"

// Standard
#include <iostream>
#include <functional>