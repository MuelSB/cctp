#include "Octahedral.hlsl"
#include "Common.hlsl"

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
    float3 ProbePositionWS : PROBE_POSITION_WS;
    float3 WorldPosition : POSITION_WS;
};

SamplerState pointBorderSampler : register(s0, space0);
Texture2D<float4> shadowMap : register(t0);
RWTexture2D<float4> probeData[2] : register(u0);

float CalculateShadow(float4 lightSpacePosition, float bias, float LoN)
{
    // Cascaded shadow maps can be implemented to reduce swimming at the edges of the map
    // Alternatively, the shadow map can be raytraced with DXR for an accurate shadow map 
    
    float2 projectedCoord;
    projectedCoord.x = lightSpacePosition.x / lightSpacePosition.w / 2.0 + 0.5;
    projectedCoord.y = -lightSpacePosition.y / lightSpacePosition.w / 2.0 + 0.5;

    float currentDepth = lightSpacePosition.z / lightSpacePosition.w;
    
    if (currentDepth > 1.0)
        return 1.0;
    
    if ((saturate(projectedCoord.x) == projectedCoord.x) && (saturate(projectedCoord.y) == projectedCoord.y))
    {
        // Percentage closer filtering for soft shadows
        const float minShadowBias = 0.001;
        float shadow = 0.0;
        float2 shadowMapDims;
        shadowMap.GetDimensions(shadowMapDims.x, shadowMapDims.y);
        float2 texelSize = 1.0 / shadowMapDims;
        float currentDepth = lightSpacePosition.z / lightSpacePosition.w;
        bias = max(bias * (1.0 - LoN), minShadowBias);
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float pcfDepth = shadowMap.Sample(pointBorderSampler, projectedCoord + float2(x, y) * texelSize).r;
                shadow += currentDepth - bias < pcfDepth ? 1.0 : 0.0;
            }
        }
        return shadow /= 9.0;
    }
    
    return 1.0;
}

float3 Lighting(float3 vertexNormalWS, float3 lightVectorWS, float3 cameraVectorWS, float shadow, float3 giIrradiance, float giVisibility)
{
    // Ambient
    const float3 ambient = float3(0.0, 0.0, 0.0);
    
    // Diffuse
    const float3 lightColor = float3(1.0, 1.0, 1.0);
    const float3 diffuse = lightColor * saturate(dot(vertexNormalWS, lightVectorWS));
    
    // Specular
    const float3 H = normalize(lightVectorWS + cameraVectorWS);
    const float NoH = saturate(dot(vertexNormalWS, H));
    const float gloss = 256.0;
    const float3 specColor = float3(0.4, 0.4, 0.4);
    const float3 specular = specColor * pow(NoH, gloss);
    
    return (shadow * (diffuse + specular) + ambient) /*+ (giIrradiance * giVisibility)*/;
}

float4 main(VertexOut input) : SV_TARGET
{
    // Calculate the direction from the shaded point to the probe
    float3 probeDirection = normalize(input.ProbePositionWS - input.WorldPosition);
    // Encode the direction to oct texture coordinate in [0, 1] range
    float2 normalizedOctCoordZeroOne = (OctEncode(probeDirection) + 1.0) * 0.5;
    // Calculate the oct coordinate in the dimensions of the probe texture
    float2 normalizedOctCoordIrradianceTextureDimensions = (normalizedOctCoordZeroOne * (float) PROBE_WIDTH_IRRADIANCE);
    float2 normalizedOctCoordVisibilityTextureDimensions = (normalizedOctCoordZeroOne * (float) PROBE_WIDTH_VISIBILITY);
    // Calculate the top left texel of this probe's data in the texture
    float2 probeTopLeftPosition = float2((float) PADDING, (float) PADDING);
    // Read irradiance
    float3 giIrradiance = probeData[0][probeTopLeftPosition + normalizedOctCoordIrradianceTextureDimensions].rgb;
    // Read visibility
    float giVisibility = probeData[1][probeTopLeftPosition + normalizedOctCoordVisibilityTextureDimensions].r;

    const float shadowBias = 0.05;
    const float4 baseColor = input.BaseColor;
    
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    if (input.Lit)
    {
        finalColor = float4(baseColor.xyz * Lighting(input.VertexNormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                CalculateShadow(input.LightSpacePosition, shadowBias, saturate(dot(input.LightVectorWS, input.VertexNormalWS))),
                                                giIrradiance,
                                                giVisibility),
                            baseColor.a);
    }
    else
    {
        finalColor = baseColor;
    }

    return finalColor;
}