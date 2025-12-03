#ifndef GBUFFER_GLSL
#define GBUFFER_GLSL

#define GBUFFER_POSITION_BINDING 0
#define GBUFFER_NORMAL_BINDING 1
#define GBUFFER_MATERIAL_BINDING 2
#define GBUFFER_ALBEDO_BINDING 3

layout(set = 2, binding = GBUFFER_POSITION_BINDING) uniform texture2D u_GBufferPosition;
layout(set = 2, binding = GBUFFER_NORMAL_BINDING) uniform texture2D u_GBufferNormal;
layout(set = 2, binding = GBUFFER_MATERIAL_BINDING) uniform texture2D u_GBufferMaterial;
layout(set = 2, binding = GBUFFER_ALBEDO_BINDING) uniform texture2D u_GBufferAlbedo;


vec3 GetGBufferPosition(vec2 uv)
{
    return texture(sampler2D(u_GBufferPosition, SAMPLER[0]), uv).xyz;
}
vec3 GetGBufferNormal(vec2 uv)
{
    return texture(sampler2D(u_GBufferNormal, SAMPLER[0]), uv).xyz;
}
vec3 GetGBufferAlbedo(vec2 uv)
{
    return texture(sampler2D(u_GBufferAlbedo, SAMPLER[0]), uv).xyz;
}
float GetGBufferMetalness(vec2 uv)
{
    return texture(sampler2D(u_GBufferMaterial, SAMPLER[0]), uv).y;
}
float GetGBufferRoughness(vec2 uv)
{
    return texture(sampler2D(u_GBufferMaterial, SAMPLER[0]), uv).x;
}
#endif