struct VertexOut
{
    float4 FinalPosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
};

struct GBuffer
{
    float4 SceneColor;
    float SceneDepth;
};

SamplerState samp : register(s0);
Texture2D<float4> shaderResources[2] : register(t0);

void SetupGBuffer(inout GBuffer gbuffer, in float2 textureCoordinate)
{
    gbuffer.SceneColor = shaderResources[0].Sample(samp, textureCoordinate);
    gbuffer.SceneDepth = shaderResources[1].Sample(samp, textureCoordinate);
}

float4 main(VertexOut input) : SV_TARGET
{
    float2 textureCoordinate = float2(input.TextureCoordinate.x, -input.TextureCoordinate.y);

    GBuffer gbuffer;
    SetupGBuffer(gbuffer, textureCoordinate);
    
    return gbuffer.SceneColor;

    //return float4(gbuffer.SceneDepth, gbuffer.SceneDepth, gbuffer.SceneDepth, 1.0);
}