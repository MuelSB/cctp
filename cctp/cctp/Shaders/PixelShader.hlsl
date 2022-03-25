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
    // Get 8 probes caging shading point
    //int probeIds[8];
    //int currentProbeId = 0;
    //for (int i = 0; i < (int) packedData.x; ++i) // Sample each probe in the field
    //{
    //    if (length(shadingPoint - ProbePositionsWS[i].rgb) <= MAX_DISTANCE)
    //    {
    //        probeIds[currentProbeId] = i;
    //        ++currentProbeId;
    //        if(currentProbeId == 8)
    //        {
    //            break;
    //        }
    //    }
    //}

    // Calculate total irradiance from 8 adjacent probes
    float3 irradiance = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < (int) packedData.x; ++i) // Sample each probe in the field
    {
        float3 probePosition = ProbePositionsWS[i].rgb;

        // Reverse the direction to the direction used to store irradiance as GI should be applied to the opposite side of the probe for reflection
        float3 probeToPoint = shadingPoint - probePosition;
        float3 direction = normalize(probeToPoint);

        // Sample irradiance and visibility from this probe
        float2 irradianceTexelIndex = GetProbeTexelCoordinate(direction, i, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING);
        float2 visibilityTexelIndex = GetProbeTexelCoordinate(direction, i, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING);

        float3 probeIrradiance = float3(0.0, 0.0, 0.0);
        probeIrradiance = irradianceData.SampleLevel(linearSampler, irradianceTexelIndex / float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT), 0);
        
        irradiance += probeIrradiance;
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
        float shadow = CalculateShadow(input.LightSpacePosition, SHADOW_BIAS, saturate(dot(input.LightVectorWS, input.NormalWS)), shadowMap, pointSampler);
        finalColor = float4(baseColor.xyz * Lighting(
                                                input.NormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                shadow,
                                                packedData.z),
                            baseColor.a);

        // Diffuse global illumination
        finalColor.rgb = finalColor.rgb + Irradiance(input.WorldPosition, input.NormalWS);
        //finalCol//.rgb = Irradiance(input.WorldPosition, input.NormalWS);
    }
    else
    {
        finalColor = baseColor;
    }

    // Gamma correct
    float gamma = 2.2;
    finalColor = pow(finalColor, 1.0 / gamma);
    
    return finalColor;
}