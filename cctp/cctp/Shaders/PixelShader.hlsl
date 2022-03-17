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
    float3 NormalWS : NORMAL_WS;
    float3 LightVectorWS : LIGHT_VECTOR_WS;
    uint Lit : Lit;
    float4 LightSpacePosition : POSITION_LS;
    float3 WorldPosition : POSITION_WS;
};

SamplerState pointSampler : register(s0, space0);
SamplerState linearSampler : register(s0, space1);
Texture2D<float> shadowMap : register(t0);
Texture2D<float3> irradianceData : register(t1);
Texture2D<float2> visibilityData : register(t2);

float Square(float x)
{
    return x * x;
}

float3 Irradiance(float3 shadingPoint, float3 shadingPointNormal)
{
    // Calculate total irradiance from 8 adjacent probes
    float3 irradiance = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < (int) packedData.x; ++i) // Sample each probe in the field
    {
        float3 probePosition = ProbePositionsWS[i].rgb;

        // Reverse the direction to the direction used to store irradiance as GI should be applied to the opposite side of the probe for reflection
        float3 pointToProbe = probePosition - shadingPoint;
        //float3 probeToPoint = shadingPoint - probePosition;
        float3 direction = normalize(pointToProbe);

        float distance = min(MAX_DISTANCE, length(pointToProbe));
        
        // Sample irradiance and visibility from this probe
        float2 irradianceTexelIndex = GetProbeTexelCoordinate(direction, i, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING);
        float2 visibilityTexelIndex = GetProbeTexelCoordinate(direction, i, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING);

        float3 probeIrradiance = float3(0.0, 0.0, 0.0);
        probeIrradiance = irradianceData.SampleLevel(linearSampler, irradianceTexelIndex / float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), 0);

        float weight = (dot(direction, shadingPointNormal) + 1.0) * 0.5;
        
        weight *= lerp(0.0, 1.0, distance);
        
        //float visibility = visibilityData[visibilityTexelIndex].r;
        //float visibility2 = visibilityData[visibilityTexelIndex].g;
        
        //float2 temp = visibilityData[visibilityTexelIndex].rg;
        //float mean = temp.r, mean2 = temp.g; // These are not currently the mean average of a bunch of rays, they are a single ray's result
        //if(distance > mean)
        //{
        //    float variance = abs(Square(mean) - mean2);
        //    weight *= variance / (variance + Square(distance - mean));
        //}
        
        irradiance += probeIrradiance * weight;
    }

    return irradiance;
}

float4 main(VertexOut input) : SV_TARGET
{
    const float4 baseColor = input.BaseColor;

    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    [branch]
    if (input.Lit)
    {
        // Light and shadow the point
        finalColor = float4(baseColor.xyz * Lighting(
                                                input.NormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                CalculateShadow(input.LightSpacePosition, SHADOW_BIAS, saturate(dot(input.LightVectorWS, input.NormalWS)), shadowMap, pointSampler),
                                                packedData.z),
                            baseColor.a);

        // Diffuse global illumination
        float ddgiPower = 0.2;
        finalColor.rgb = finalColor.rgb + Irradiance(input.WorldPosition, input.NormalWS) * ddgiPower;
        //finalColor.rgb = Irradiance(input.WorldPosition, input.NormalWS);
    }
    else
    {
        finalColor = baseColor;
    }

    return finalColor;
}