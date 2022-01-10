struct VertexOut
{
    float4 FinalPosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
};

SamplerState samp : register(s0);
Texture2D<float4> shaderResources[1] : register(t0);

float4 main(VertexOut input) : SV_TARGET
{
    return shaderResources[0].Sample(samp, float2(input.TextureCoordinate.x, -input.TextureCoordinate.y));
}