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
        13,18,23,13,23,11,9,21,15,21,17,15,12,14,19,14,16,19,7,6,22,6,20,22,5,4,10,4,8,10,3,2,0,2,1,0
    };
}
