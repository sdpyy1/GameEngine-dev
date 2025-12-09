#version 450 core
#include "../common/common.glsl"
#include "../common/DDGI.glsl"
#include "../common/gizmo.glsl"

layout(set = 1, rgba32f, binding = 0) uniform writeonly image2DArray o_Texture;
layout(set = 1, binding = 1) uniform texture2DArray  IN_RayData;
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
    uint probeIndex = DDGIGetProbeIndex(invocationID, DDGI_PROBE_NUM_TEXELS_IRRANDIANCE, volume);

    // Early out: no probe maps to this thread
    uint numProbes = (volume.probeCount.x * volume.probeCount.y * volume.probeCount.z);
    if (probeIndex >= numProbes || probeIndex < 0) return;

    vec4 result = vec4(0.0);

    // 不是边界，就需要计算了 
    if(!isBorderTexel){
        // 因为local_size_z = 1，所以invocationID.z存储的就是层数
        // uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, groupID.y * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, invocationID.z) 存储的是这个探针存储区域的起始位置
        // + LocalInvocationID 加上这个线程是处理8*8区域中的哪个
        //  - uvec3(1, 1, 0) 排除掉边界（这个逻辑是在!isBorderTexel才生效，因为0，0，0不会进来这里）
        // 最终threadCoords就是当前线程要处理的哪个探针的6*6区域中的哪个位置（排除边界）
        uvec3 threadCoords = uvec3(groupID.x * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, groupID.y * DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR, invocationID.z) + LocalInvocationID - uvec3(1, 1, 0);

        // 一个6*6的区域，获取某个像素对应到八面体上的UV坐标
        vec2 probeOctantUV = DDGIGetNormalizedOctahedralCoordinates(uvec2(threadCoords.xy), DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR);
        // 当前像素对应投影到八面体上的射线方向
        vec3 probeRayDirection = DDGIGetOctahedralDirection(probeOctantUV);

        uvec3 probeCoords = DDGIGetProbeCoords(probeIndex,volume);

        // if(volume.visulaize == 1 && probeCoords == uvec3(4,4,4)){
        //     vec3 probeWorldPosition = DDGIGetProbeWorldPosition(probeCoords, volume);
        //     AddGizmoLine(probeWorldPosition, probeWorldPosition + probeRayDirection*3, vec4(0,1,0,1));
        // }

    
        // 遍历当前探针的所有光线
        for (uint rayIndex = 0; rayIndex < volume.raysPerProbe; rayIndex++){
            // 获取ray的方向
            vec3 rayDirection = normalize(RTXGISphericalFibonacci(rayIndex,volume.raysPerProbe));
            // 很好理解，当前要存储的方向与这跟光线的cos，两个方向夹角越小，权重越大
            float weight = max(0.0, dot(probeRayDirection, rayDirection));
            // 解析RayData中的信息
            uvec3 rayDataTexCoords = DDGIGetRayDataTexelCoords(rayIndex, probeIndex, volume);
            vec3 probeRayRadiance = DDGIGetRandianceFromRayData(IN_RayData, rayDataTexCoords, volume);
            float probeRayDistance = DDGIGetDistanceFromRayData(IN_RayData, rayDataTexCoords, volume);

            // if(probeRayDistance < 0){ 
            //     continue;
            // }
            
            if(volume.visulaize == 1 && probeCoords == uvec3(4,4,4)){
                vec3 probeWorldPosition = DDGIGetProbeWorldPosition(probeCoords, volume);
                AddGizmoLine(probeWorldPosition, probeWorldPosition - rayDirection, vec4(0, 1, 0,1));
            }
            result += vec4(probeRayRadiance * weight, weight);
        }
        float epsilon = float(volume.raysPerProbe);
        epsilon *= 1e-9f;
        result.rgb *= 1.f / (2.f * max(result.a, epsilon));
        result.a = 1.f;
    imageStore(o_Texture, ivec3(gl_GlobalInvocationID), result);
    }else{
        // 边界
        imageStore(o_Texture, ivec3(gl_GlobalInvocationID), vec4(0,0,0,1));
    }
}

#endif