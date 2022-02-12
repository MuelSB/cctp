#include "Common.hlsl"

#define MAX_MATERIALS 7

cbuffer MaterialBuffer : register(b0)
{
    float4 Colors[MAX_MATERIALS];
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    // Sample color of hit object
    uint hitInstanceID = InstanceID();
    
    // TODO Shade the hit point as in forward lighting pass to calculate radiance
    
    payload.HitIrradiance = Colors[hitInstanceID].xyz;
    payload.HitDistance = RayTCurrent();
}