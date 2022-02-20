#include "Common.hlsl"

[shader("miss")]
void Miss(inout RayPayload payload)
{
    payload.HitIrradiance = float3(0.0f, 0.0f, 0.0f);
    payload.HitDistance = 1e38f;
}