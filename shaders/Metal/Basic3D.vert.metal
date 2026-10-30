#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float3 position [[attribute(0)]];
    float3 color [[attribute(1)]];
};

struct VertexOutput {
    float4 position [[position]];
    float3 color;
};

struct CameraConstants {
    float4x4 viewProjection;
};

vertex VertexOutput main_vertex(
    VertexInput input [[stage_in]],
    constant CameraConstants& camera [[buffer(0)]])
{
    VertexOutput output;
    output.position = camera.viewProjection * float4(input.position, 1.0);
    output.color = input.color;
    return output;
}
