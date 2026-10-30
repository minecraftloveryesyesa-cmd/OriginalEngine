float4 main(float3 Color : TEXCOORD0) : SV_Target0
{
    return float4(saturate(Color), 1.0f);
}
