#version 450 core
#include "common/common.glsl"
#include "common/constant.glsl"
#ifdef COMPUTE_SHADER
layout(set = 2, rgba32f, binding = 0) uniform writeonly image2D o_Texture;
layout(set = 2, binding = 1) uniform texture2D u_InputTexture;
layout(set = 2, binding = 2) uniform texture2D allDonwTexture;
layout(set = 1, binding = 0) uniform sampler SAMPLER[];
layout(push_constant) uniform Uniforms
{
    vec4 Params; // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
    float LOD;
    int Mode;
} u_Uniforms;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
#define MODE_PREFILTER      0
#define MODE_DOWNSAMPLE     1
#define MODE_UPSAMPLE_FIRST 2
#define MODE_UPSAMPLE       3

// ����
vec3 DownsampleBox13(texture2D tex, float lod, vec2 uv, vec2 texelSize, sampler samplr)
{
        // Center
    vec3 A = textureLod(sampler2D(tex,samplr), uv, lod).rgb;

    texelSize *= 0.5f; // Sample from center of texels

    // Inner box
    vec3 B = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(-1.0f, -1.0f), lod).rgb;
    vec3 C = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(-1.0f, 1.0f), lod).rgb;
    vec3 D = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(1.0f, 1.0f), lod).rgb;
    vec3 E = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(1.0f, -1.0f), lod).rgb;

    // Outer box
    vec3 F = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(-2.0f, -2.0f), lod).rgb;
    vec3 G = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(-2.0f, 0.0f), lod).rgb;
    vec3 H = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(0.0f, 2.0f), lod).rgb;
    vec3 I = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(2.0f, 2.0f), lod).rgb;
    vec3 J = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(2.0f, 2.0f), lod).rgb;
    vec3 K = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(2.0f, 0.0f), lod).rgb;
    vec3 L = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(-2.0f, -2.0f), lod).rgb;
    vec3 M = textureLod(sampler2D(tex,samplr), uv + texelSize * vec2(0.0f, -2.0f), lod).rgb;

    // Weights
    vec3 result = vec3(0.0);
    // Inner box
    result += (B + C + D + E) * 0.5f;
    // Bottom-left box
    result += (F + G + A + M) * 0.125f;
    // Top-left box
    result += (G + H + I + A) * 0.125f;
    // Top-right box
    result += (A + I + J + K) * 0.125f;
    // Bottom-right box
    result += (M + A + K + L) * 0.125f;

    // 4 samples each
    result *= 0.25f;

    return result;
}

// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
vec4 QuadraticThreshold(vec4 color, float threshold, vec3 curve)
{
    // Maximum pixel brightness
    float brightness = max(max(color.r, color.g), color.b);
    // Quadratic curve
    float rq = clamp(brightness - curve.x, 0.0, curve.y);
    rq = (rq * rq) * curve.z;
    color *= max(rq, brightness - threshold) / max(brightness, Epsilon);
    return color;
}

vec4 Prefilter(vec4 color, vec2 uv)
{
    float clampValue = 20.0f;
    color = clamp(color, vec4(0.0f), vec4(clampValue));
    color = QuadraticThreshold(color, u_Uniforms.Params.x, u_Uniforms.Params.yzw);
    return color;
}

vec3 UpsampleTent9(texture2D tex, float lod, vec2 uv, vec2 texelSize, float radius, sampler samplr)
{
    vec4 offset = texelSize.xyxy * vec4(1.0f, 1.0f, -1.0f, 0.0f) * radius;

    // ���Ĳ���
    vec3 result = textureLod(sampler2D(tex, samplr), uv, lod).rgb * 4.0f;

    result += textureLod(sampler2D(tex, samplr), uv - offset.xy, lod).rgb;
    result += textureLod(sampler2D(tex, samplr), uv - offset.wy, lod).rgb * 2.0;
    result += textureLod(sampler2D(tex, samplr), uv - offset.zy, lod).rgb;

    result += textureLod(sampler2D(tex, samplr), uv + offset.zw, lod).rgb * 2.0;
    result += textureLod(sampler2D(tex, samplr), uv + offset.xw, lod).rgb * 2.0;

    result += textureLod(sampler2D(tex, samplr), uv + offset.zy, lod).rgb;
    result += textureLod(sampler2D(tex, samplr), uv + offset.wy, lod).rgb * 2.0;
    result += textureLod(sampler2D(tex, samplr), uv + offset.xy, lod).rgb;

    return result * (1.0f / 16.0f);
}

void main()
{
// ���Pass�Ѿ������ò���������~
    vec2 imgSize = vec2(imageSize(o_Texture));
    ivec2 invocID = ivec2(gl_GlobalInvocationID);
    if (invocID.x >= imgSize.x || invocID.y >= imgSize.y) 
        return;
        
    vec2 texCoords = vec2(float(invocID.x) / imgSize.x, float(invocID.y) / imgSize.y);
    texCoords += (1.0f / imgSize) * 0.5f; // ��ȡ������������

    vec2 texSize = vec2(textureSize(u_InputTexture, int(u_Uniforms.LOD)));
    vec4 color = vec4(0, 0, 0, 1);
    
    if (u_Uniforms.Mode == MODE_PREFILTER)
    {
        color.rgb = DownsampleBox13(u_InputTexture, 0, texCoords, 1.0f / texSize, SAMPLER[0]);
        color = Prefilter(color, texCoords);
        color.a = 1.0f;
    }
    else if (u_Uniforms.Mode == MODE_DOWNSAMPLE)
    {
        color.rgb = DownsampleBox13(u_InputTexture, u_Uniforms.LOD, texCoords, 1.0f / texSize, SAMPLER[0]);
    }
    else if (u_Uniforms.Mode == MODE_UPSAMPLE)
    {
        // ����һ�������������
        vec2 bloomTexSize = vec2(textureSize(u_InputTexture, 0));
        float sampleScale = 1.0f;
        vec3 upsampledTexture = UpsampleTent9(u_InputTexture,0, texCoords, 1.0f / bloomTexSize, sampleScale, SAMPLER[0]);

        // ���ϱ��������е�
        vec3 existing = textureLod(sampler2D(allDonwTexture, SAMPLER[0]), texCoords,u_Uniforms.LOD).rgb;
        color.rgb = existing + upsampledTexture;
    }
    
    imageStore(o_Texture, ivec2(gl_GlobalInvocationID), color);
}

#endif