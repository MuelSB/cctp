#include "Common.hlsl"

#define MAX_MATERIALS 7

struct Vertex1Pos1UV1Norm
{
    float4 Position;
    float4 UV;
    float4 Normal;
};

cbuffer MaterialBuffer : register(b0)
{
    float4 Colors[MAX_MATERIALS];
}

cbuffer PerFrameConstants : register(b1)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    int ProbeCount;
}

cbuffer PerPassConstants : register(b2)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4 CameraPositionWS;
}

SamplerState pointBorderSampler : register(s0);
Texture2D<float4> shadowMap : register(t0);
StructuredBuffer<Vertex1Pos1UV1Norm> cubeVertices : register(t1);

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    // Sample color of hit object
    uint hitInstanceID = InstanceID();
    
    // Calculate hit point in world space
    float3 shadingPointWS = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());
    
    // Get the base triangle index in the geometry object
    uint triangleIndex = PrimitiveIndex();

    Vertex1Pos1UV1Norm v0 = cubeVertices[triangleIndex];
    Vertex1Pos1UV1Norm v1 = cubeVertices[triangleIndex + 1];
    Vertex1Pos1UV1Norm v2 = cubeVertices[triangleIndex + 2];

    // Interpolate normal attribute
    float3 normalWS = attribs.barycentrics.x * (mul((float3x3)ObjectToWorld3x4(), v0.Normal.xyz)) + 
        attribs.barycentrics.y * (mul((float3x3)ObjectToWorld3x4(), v1.Normal.xyz)) +
        attribs.barycentrics.g * (mul((float3x3)ObjectToWorld3x4(), v2.Normal.xyz));

    float3 lightVectorWS = -normalize(LightDirectionWS.xyz);
    float3 cameraVectorWS = normalize(CameraPositionWS.xyz - shadingPointWS);
    
    float shadow = CalculateShadow(
        mul(LightMatrix, float4(shadingPointWS, 1.0)),
        SHADOW_BIAS,
        saturate(dot(lightVectorWS, normalWS)),
        shadowMap,
        pointBorderSampler
    );
    
    float3 lighting = Lighting(
        normalWS,
        lightVectorWS,
        cameraVectorWS,
        shadow
    );
    
    payload.HitIrradiance = Colors[hitInstanceID].xyz * lighting;
    payload.HitDistance = RayTCurrent();
}