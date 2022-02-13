#include "Octahedral.hlsl"
#include "Common.hlsl"

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    int ProbeCount;
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

SamplerState pointBorderSampler : register(s0);
Texture2D<float4> textureResources[3] : register(t0);

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
        textureResources[0].GetDimensions(shadowMapDims.x, shadowMapDims.y);
        float2 texelSize = 1.0 / shadowMapDims;
        float currentDepth = lightSpacePosition.z / lightSpacePosition.w;
        bias = max(bias * (1.0 - LoN), minShadowBias);
        for (int x = -1; x <= 1; ++x)
        {
            for (int y = -1; y <= 1; ++y)
            {
                float pcfDepth = textureResources[0].Sample(pointBorderSampler, projectedCoord + float2(x, y) * texelSize).r;
                shadow += currentDepth - bias < pcfDepth ? 1.0 : 0.0;
            }
        }
        return shadow /= 9.0;
    }
    
    return 1.0;
}

float3 Lighting(float3 vertexNormalWS, float3 lightVectorWS, float3 cameraVectorWS, float shadow)
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
    
    return shadow * (diffuse + specular) + ambient;
}

float4 main(VertexOut input) : SV_TARGET
{
    const float shadowBias = 0.05;
    const float4 baseColor = input.BaseColor;
    
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    if (input.Lit)
    {
        // Light and shadow the point
        finalColor = float4(baseColor.xyz * Lighting(input.VertexNormalWS,
                                                input.LightVectorWS,
                                                input.CameraVectorWS,
                                                CalculateShadow(input.LightSpacePosition, shadowBias, saturate(dot(input.LightVectorWS, input.VertexNormalWS)))),
                            baseColor.a);

        // Global illumination
        float3 irradiance = float3(0.0, 0.0, 0.0);

        for (int p = 0; p < ProbeCount; ++p)
        {
            float3 dir = input.WorldPosition - ProbePositionsWS[p].xyz;
            float r = length(dir);

            if (r < /* probe spacing */ 2.0)
            {
                // This is one of the 8 probes around the shaded point
                dir = normalize(dir);
                
                // Smooth backface
                float weight = (dot(dir, input.VertexNormalWS) + 1) * 0.5;
                
                // Adjacency
                // TODO Trilinear interpolation
                // Weight probe contribution that is nearer to the shaded point higher

                // Visibility
                float2 temp = textureResources[2][GetProbeTextureCoord(dir, p, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING)].rg;

                irradiance += textureResources[1][GetProbeTextureCoord(dir, p, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING)].rgb * weight * temp.r;
            }
        }
        
        finalColor += float4(irradiance, 0.0);
    }
    else
    {
        finalColor = baseColor;
    }

    return finalColor;
}