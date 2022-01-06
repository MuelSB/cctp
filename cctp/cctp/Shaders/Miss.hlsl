#include "Common.hlsl"

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.HitIrradiance = float3(0.0, 0.0, 0.0);
    payload.HitDistance = 1.0;
}