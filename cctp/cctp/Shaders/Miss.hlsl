#include "Common.hlsl"

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.HitColor.b = 1.0f;
}