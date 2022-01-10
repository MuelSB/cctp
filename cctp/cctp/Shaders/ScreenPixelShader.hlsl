struct VertexOut
{
    float4 FinalPosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
};



float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}