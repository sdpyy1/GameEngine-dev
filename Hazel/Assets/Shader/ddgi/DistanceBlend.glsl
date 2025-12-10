#version 450 core
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
layout(set = 1, rgba32f, binding = 0) uniform image2DArray o_Texture;
layout(set = 1, binding = 1) uniform texture2DArray  IN_RayData;
#ifdef COMPUTE_SHADER
layout(local_size_x = DDGI_PROBE_NUM_TEXELS_DISTANCE, local_size_y = DDGI_PROBE_NUM_TEXELS_DISTANCE, local_size_z = 1) in;  
void main(){
	DDGISetting volume = GetDDGISetting();
    uvec3 groupID = gl_WorkGroupID;  // 对于dispatch的ID
    uvec3 invocationID = gl_GlobalInvocationID; // 相对于全局的调用ID
    uvec3 LocalInvocationID = gl_LocalInvocationID; // 相对于组内的调用ID

    bool isBorderTexel = (LocalInvocationID.x == 0 || LocalInvocationID.x == (DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR + 1)) || (LocalInvocationID.y == 0 || LocalInvocationID.y == (DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR + 1)); // Border Columns

    uint probeIndex = DDGIGetProbeIndex(invocationID, DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, volume);

    vec4 result = vec4(0.0);

    if(!isBorderTexel){
        uvec3 threadCoords = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR, groupID.y * DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR, invocationID.z) + LocalInvocationID - uvec3(1, 1, 0);

        // 一个6*6的区域，获取某个像素对应到八面体上的UV坐标
        vec2 probeOctantUV = DDGIGetNormalizedOctahedralCoordinates(uvec2(threadCoords.xy), DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR);
        // 当前像素对应投影到八面体上的射线方向
        vec3 probeRayDirection = DDGIGetOctahedralDirection(probeOctantUV);

        // 遍历当前探针的所有光线
        for (uint rayIndex = 0; rayIndex < volume.raysPerProbe; rayIndex++){
            vec3 rayDirection = normalize(RTXGISphericalFibonacci(rayIndex, volume.raysPerProbe));
            float weight = max(0.f, dot(probeRayDirection, rayDirection));
            uvec3 rayDataTexCoords = DDGIGetRayDataTexelCoords(rayIndex, probeIndex, volume);
            float probeMaxRayDistance = length(volume.gridStep) * 1.5f;
            float probeRayDistance = 0.f;
            probeRayDistance = min(abs(DDGIGetDistanceFromRayData(IN_RayData, rayDataTexCoords, volume)), probeMaxRayDistance);
            result += vec4(probeRayDistance * weight, (probeRayDistance * probeRayDistance) * weight, 0.f, weight);
        }
        float epsilon = float(volume.raysPerProbe);
        epsilon *= 1e-9f;
        result.rgb *= 1.f / (2.f * max(result.a, epsilon));
        result.a = 1.f;
        // 时域加权混合
        vec4 history = imageLoad(o_Texture,ivec3(gl_GlobalInvocationID));
        result = mix(result, history, RGBtoLuminance(history) < 0.01f ? 0.0f : 0.97);
        result.a = 1.0f;
        result.b = 1.0f;
        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), result);
    }else{
        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), result);
    }

}

#endif