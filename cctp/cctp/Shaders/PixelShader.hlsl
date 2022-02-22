#include "Octahedral.hlsl"
#include "Common.hlsl"

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    float4 packedData; // Stores probe count (x), probe spacing (y), light intensity (z)
}

struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
    float4 BaseColor : BASE_COLOR;
    float3 CameraVectorWS : CAMERA_VECTOR_WS;
    float3 VertexNormalWS : NORMAL_WS;
    float3 LightVectorWS : LIGHT_VECTOR_WS;
    uint Lit : Lit;
    float4 LightSpacePosition : POSITION_LS;
    float3 WorldPosition : POSITION_WS;
};

SamplerState pointSampler : register(s0, space0);
SamplerState linearSampler : register(s0, space1);
Texture2D<float> shadowMap : register(t0);
Texture2D<float3> irradianceData : register(t1);
Texture2D<float> visibilityData : register(t2);

void GetAdjacentProbeIndices(in float3 position, out uint indices[8])
{
    uint currentAdjacentProbeIndex = 0;
    for (int p = 0; p < packedData.x /* Probe count */; ++p)
    {
        float3 probePosition = ProbePositionsWS[p].xyz;
        float3 dir = ProbePositionsWS[p].xyz - position;
        if (length(dir) <= packedData.y /* Probe spacing */)
        {
            indices[currentAdjacentProbeIndex] = p;
            ++currentAdjacentProbeIndex;
            if (currentAdjacentProbeIndex == 8)
                break;
        }
    }
}

float4 Irradiance(float3 shadingPoint, float3 N)
{
    // Get 8 adjacent probes to the shading point
    uint adjacentProbeIndices[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    GetAdjacentProbeIndices(shadingPoint, adjacentProbeIndices);

    // For each adjacent probe around the shading point
    return float4(0.0, 0.0, 0.0, 0.0);
}

float4 main(VertexOut input) : SV_TARGET
{    
    const float4 baseColor = input.BaseColor;
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    [branch]
    if (input.Lit)
    {
        // Debug

        float3 shadingPoint = input.WorldPosition;
        float3 probePosition = ProbePositionsWS[0];
        float3 pointToProbe = probePosition - shadingPoint; // Reverse the direction to the direction used to store irradiance as GI should be applied to the opposite side of the probe for reflection
        float3 direction = normalize(pointToProbe);
        float3 irradiance = irradianceData[GetProbeTextureCoord(direction, 0, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING)].rgb;
        //float visibility = textureResources[2][GetProbeTextureCoord(direction, 0, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING)].r;
    
        ////////

        // Light and shadow the point
        finalColor = float4(baseColor.xyz * saturate(Lighting(
                                                input.VertexNormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                CalculateShadow(input.LightSpacePosition, SHADOW_BIAS, saturate(dot(input.LightVectorWS, input.VertexNormalWS)), shadowMap, pointSampler),
                                                packedData.z)),
                            baseColor.a);
        
        // Diffuse global illumination
        float mask = packedData.y - pow(length(pointToProbe), 0.4f);
        finalColor.rgb = saturate(finalColor.rgb + irradiance * mask);
    }
    else
    {
        finalColor = baseColor;
    }

    return finalColor;
}