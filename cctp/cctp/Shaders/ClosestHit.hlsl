#include "Common.hlsl"

#define MAX_MATERIALS 6

cbuffer MaterialBuffer : register(b0)
{
    float4 Colors[MAX_MATERIALS];
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    // Sample color of hit object
    uint hitInstanceID = InstanceID();
    
    payload.HitColor = Colors[hitInstanceID].xyz;
}