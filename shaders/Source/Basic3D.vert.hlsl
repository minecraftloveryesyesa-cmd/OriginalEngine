cbuffer CameraConstants : register(b0)
{
    float4x4 ViewProjection;
};

struct VertexInput
{
    float3 Position : TEXCOORD0;
    float3 Color : TEXCOORD1;
};

struct VertexOutput
{
    float3 Color : TEXCOORD0;
    float4 Position : SV_Position;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.Position = mul(float4(input.Position, 1.0f), ViewProjection);
    output.Color = input.Color;
    return output;
}
