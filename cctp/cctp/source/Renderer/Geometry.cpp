#include "Pch.h"
#include "Geometry.h"

void Renderer::Geometry::GenerateCubeGeometry(std::vector<Vertex1Pos1UV1Norm>& outVertices, std::vector<uint32_t>& outIndices, const float width)
{
    const float halfWidth = width / 2.0f;

    outVertices =
    {
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth, -halfWidth) , glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,   halfWidth, -halfWidth), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(-halfWidth,  -halfWidth, halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,   halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) ),
    Vertex1Pos1UV1Norm(glm::vec3(halfWidth,  -halfWidth,  halfWidth) , glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    outIndices =
    {
        0,1,2,0,2,3,10,8,4,10,4,5,22,20,6,22,6,7,19,16,14,19,14,12,15,17,21,15,21,9,11,23,13,23,18,13
    };
}
