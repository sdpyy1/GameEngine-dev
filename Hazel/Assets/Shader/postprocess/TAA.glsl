#version 450 core
#include "../common/common.glsl"
#include "../common/TAA.glsl"
#ifdef COMPUTE_SHADER

layout(set = 1,binding = 0,rgba32f) uniform image2D out_texture;
layout(set = 1,binding = 1) uniform texture2D velocityTexture;
layout(set = 1,binding = 2) uniform texture2D historyTexture;
layout(set = 1,binding = 3) uniform texture2D curTexture;


layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    ivec2 imgSize = ivec2(imageSize(out_texture));
    ivec2 invocID = ivec2(gl_GlobalInvocationID.xy);
    if (invocID.x >= imgSize.x || invocID.y >= imgSize.y) 
        return;


    vec2 texCoords = (vec2(invocID) + 0.5f) / vec2(imgSize);
    vec2 velocity = texture(sampler2D(velocityTexture,SAMPLER[0]), texCoords).rg;

    vec2 historyTexCoords = texCoords - velocity;
    vec3 historyColor = texture(sampler2D(historyTexture,SAMPLER[0]), historyTexCoords).rgb;
    vec3 curColor = texture(sampler2D(curTexture,SAMPLER[0]), texCoords).rgb;
    vec3 mixColor = mix(historyColor, curColor, 0.05f);
    imageStore(out_texture, invocID, vec4(mixColor, 1.0));
    
}

#endif