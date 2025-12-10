#version 450 core
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
#include "../common/gizmo.glsl"
/*
每个线程处理一个探针的8*8区域的一个像素
*/
layout(set = 1, rgba32f, binding = 0) uniform image2DArray o_Texture;
layout(set = 1, rgba32f, binding = 1) uniform image2DArray  IN_RayData;
#ifdef COMPUTE_SHADER
 // dispatch的xyz是探针分布，local_size定义每个探针需要的像素数量
 // 对于Irrandiance纹理，每个探针占据 (6+2)*(6+2) 个像素，额外两圈是为了避免采样到别的探针数据的边界填充
layout(local_size_x = DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, local_size_y = DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, local_size_z = 1) in;  // Dispach的是 ProbeCount的 xzy（Y_up）
void main(){

    // 索引探针在RayData的位置
	DDGISetting volume = GetDDGISetting();
    uvec3 groupID = gl_WorkGroupID;  // 对于dispatch的ID
    uvec3 invocationID = gl_GlobalInvocationID; // 相对于全局的调用ID
    uvec3 LocalInvocationID = gl_LocalInvocationID; // 相对于组内的调用ID

    // 判断是不是边界（边界是填充，不是计算），利用的是在本组内的索引xy，因为这个索引对应的是一个探针的8*8数据块，所以如果索引是0或者7，那么就是边界
    bool isBorderTexel = (LocalInvocationID.x == 0 || LocalInvocationID.x == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR + 1)) || (LocalInvocationID.y == 0 || LocalInvocationID.y == (DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR + 1)); // Border Columns

    // 从invocationID获取当先要处理的探针索引
    // uint probeIndex = DDGIGetProbeIndex(invocationID, DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, volume);
    uint probeIndex = (groupID.y * DDGIGetProbesPerPlane(volume.probeCount)) + groupID.z * volume.probeCount.x + groupID.x ;

    // Early out: no probe maps to this thread
    uint numProbes = (volume.probeCount.x * volume.probeCount.y * volume.probeCount.z);
    if (probeIndex >= numProbes || probeIndex < 0) return;

    vec4 result = vec4(0.0);

    // 不是边界，就需要计算了 
    if(!isBorderTexel){
        uvec3 threadCoords = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, groupID.y * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, invocationID.z) + LocalInvocationID - uvec3(1, 1, 0);
        vec2 probeOctantUV = DDGIGetNormalizedOctahedralCoordinates(uvec2(threadCoords.xy), DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR);
        vec3 probeRayDirection = DDGIGetOctahedralDirection(probeOctantUV);
    
        // 遍历当前探针的所有光线
        for (uint rayIndex = 0; rayIndex < volume.raysPerProbe; rayIndex++){
            // 获取ray的方向
            vec3 rayDirection = normalize(RTXGISphericalFibonacci(rayIndex,volume.raysPerProbe));
            // 当前要存储的方向与这跟光线的cos，两个方向夹角越小，权重越大
            float weight = max(0.0, dot(probeRayDirection, rayDirection));
            // 解析RayData中的信息
            uvec3 rayDataTexCoords = DDGIGetRayDataTexelCoords(rayIndex, probeIndex, volume);  
            vec4 storedData = imageLoad(IN_RayData, ivec3(rayDataTexCoords));
            vec3 probeRayRadiance = storedData.xyz;
            float probeRayDistance = storedData.w;
            if(probeRayDistance < 0){ 
                continue;
            }
            result += vec4(probeRayRadiance * weight, weight);
        }

        // 工程化修正问题：需要结合蒙特卡洛积分理解
        // Normalize the blended irradiance (or filtered distance), if the combined weight is not close to zero.
        // To match the Monte Carlo Estimator of Irradiance, we should divide by N (the number of radiance samples).
        // Instead, we are dividing by sum(cos(theta)) (i.e. the sum of cosine weights) to reduce variance. To account
        // for this, we must multiply in a factor of 1/2. See the Math Guide in the documentation for more information.
        // For distance, note that we are *not* dividing by the sum of the cosine weights, but to avoid branching here
        // we are still dividing by 2. This means distance values sampled from texture need to be multiplied by 2 (see
        // Irradiance.hlsl line 138).
        float epsilon = float(volume.raysPerProbe);
        epsilon *= 1e-9f;
        result.rgb *= 1.f / (2.f * max(result.a, epsilon));

        // 时域加权混合
        vec4 history = imageLoad(o_Texture,ivec3(gl_GlobalInvocationID));
        result = mix(result, history, RGBtoLuminance(history) < 0.01f ? 0.0f : 0.97); // TODO：混合系数是参数
        result.a = 1.0f;
        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), result);
    }else{
        // 边界
        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), vec4(0,0,0,1));
    }
}

#endif