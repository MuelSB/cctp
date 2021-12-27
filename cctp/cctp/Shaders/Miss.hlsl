#include "Common.hlsl"

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.HitColor = float3(0.0, 1.0, 1.0);
}