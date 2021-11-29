#include "RayPayload.hlsl"

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.HitColor.r = 1.0f;
}