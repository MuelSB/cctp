#include "Common.hlsl"

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.HitColor = float3(0.0f, 0.0f, 1.0f);
}