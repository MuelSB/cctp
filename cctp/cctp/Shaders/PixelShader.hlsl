struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
    float4 BaseColor : BASE_COLOR;
    float3 CameraVectorWS : CAMERA_VECTOR_WS;
    float3 VertexNormalWS : NORMAL_WS;
    float3 LightVectorWS : LIGHT_VECTOR_WS;
    uint Lit : Lit;
};

float3 Lighting(float3 vertexNormalWS, float3 lightVectorWS, float3 cameraVectorWS)
{
    // Ambient
    const float3 ambient = float3(0.1f, 0.1f, 0.1f);
    
    // Diffuse
    const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
    const float3 diffuse = lightColor * saturate(dot(vertexNormalWS, lightVectorWS));
    
    // Specular
    const float3 H = normalize(lightVectorWS + cameraVectorWS);
    const float NoH = saturate(dot(vertexNormalWS, H));
    const float gloss = 256.0;
    const float3 specColor = float3(0.4, 0.4, 0.4);
    const float3 specular = specColor * pow(NoH, gloss);
    
    return ambient + diffuse + specular;
}

float4 main(VertexOut input) : SV_TARGET
{
    const float4 baseColor = input.BaseColor;

    float4 finalColor = float4(0.0, 0.0, 0.0, 1.0);

    if(input.Lit)
    {
        finalColor = float4(baseColor.xyz * Lighting(input.VertexNormalWS, input.LightVectorWS, input.CameraVectorWS), baseColor.a);
    }
    else
    {
        finalColor = baseColor;
    }
    
    return finalColor;
}