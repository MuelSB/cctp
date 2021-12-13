#include "Common.hlsl"

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.HitColor = float3(1.0f, 0.0f, 0.0f);
}