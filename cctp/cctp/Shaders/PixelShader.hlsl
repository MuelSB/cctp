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
};

SamplerState pointClampSampler : register(s0);
Texture2D<float4> shadowMap : register(t0);

float CalculateShadow(float4 lightSpacePosition, float bias, float LdotN)
{
    float2 projectedCoord;
    float denom = 
    projectedCoord.x = lightSpacePosition.x / lightSpacePosition.w / 2.0 + 0.5;
    projectedCoord.y = -lightSpacePosition.y / lightSpacePosition.w / 2.0 + 0.5;
    
    if (lightSpacePosition.z > 1.0)
        return 1.0;
    
    if ((saturate(projectedCoord.x) == projectedCoord.x) && (saturate(projectedCoord.y) == projectedCoord.y))
    {
        float depth = shadowMap.Sample(pointClampSampler, projectedCoord).r;
        
        bias = max(bias * (1.0 - LdotN), 0.001);
        float lightDepth = lightSpacePosition.z / lightSpacePosition.w;
        lightDepth = lightDepth - bias;
        
        return lightDepth < depth ? 1.0 : 0.0;
    }
    
    return 1.0;
}

float3 Lighting(float3 vertexNormalWS, float3 lightVectorWS, float3 cameraVectorWS, float shadow)
{
    // Ambient
    const float3 ambient = float3(0.1, 0.1, 0.1);
    
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
    const float shadowBias = 0.0075;
    const float4 baseColor = input.BaseColor;
    
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0);

    if (input.Lit)
    {
        finalColor = float4(baseColor.xyz * Lighting(input.VertexNormalWS, 
                                                input.LightVectorWS, 
                                                input.CameraVectorWS, 
                                                CalculateShadow(input.LightSpacePosition, shadowBias, saturate(dot(input.LightVectorWS, input.VertexNormalWS)))),
                            baseColor.a);
    }
    else
    {
        finalColor = baseColor;
    }
    
    return finalColor;
}