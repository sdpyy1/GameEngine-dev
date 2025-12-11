#ifndef DDGI_GLSL
#define DDGI_GLSL
#define DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR 6u
#define DDGI_PROBE_NUM_TEXELS_IRRANDIANCE 8u
#define DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR 14u
#define DDGI_PROBE_NUM_TEXELS_DISTANCE 16u
#include "../common/gizmo.glsl"

#include "math.glsl"
/*
    获取每层探针数量
*/
uint DDGIGetProbesPerPlane(uvec3 probeCount){
    return probeCount.x * probeCount.z;
}
/*
    根据Probe的全局Index，计算他在探针网格的坐标
*/
uvec3 DDGIGetProbeCoords(uint probeIndex, DDGISetting volume)
{
    uvec3 probeCoords;
    probeCoords.x = probeIndex % volume.probeCount.x;
    probeCoords.y = probeIndex / (volume.probeCount.x * volume.probeCount.z);
    probeCoords.z = (probeIndex / volume.probeCount.x) % volume.probeCount.z;
    return probeCoords;
}
/*
    根据探针网格坐标，计算世界坐标
*/
vec3 DDGIGetProbeWorldPosition(uvec3 probeCoords, DDGISetting volume){
    vec3 probeGridWorldPosition  = probeCoords * volume.gridStep;   // 网格坐标考虑探针间距后的坐标
    vec3 probeGridShift = (volume.gridStep * (volume.probeCount - 1)) * 0.5; // 偏移探针到网格中心
    vec3 probeWorldPosition = probeGridWorldPosition - probeGridShift; // 探针在网格中的位置
    probeWorldPosition += volume.centerPosition; // 加上volume的世界位置
    return probeWorldPosition;
}

/**
 * 通过Spherical fibonacci mapping随机生成一组球面上的方向。
 * Computes a low discrepancy spherically distributed direction on the unit sphere,
 * for the given index in a set of samples. Each direction is unique in
 * the set, but the set of directions is always the same.
 */
vec3 RTXGISphericalFibonacci(float sampleIndex, float numSamples)
{
    const float b = (sqrt(5.f) * 0.5f + 0.5f) - 1.f;
    float phi = TwoPI * fract(sampleIndex * b);
    float cosTheta = 1.f - (2.f * sampleIndex + 1.f) * (1.f / numSamples);
    float sinTheta = sqrt(Saturate(1.f - (cosTheta * cosTheta)));

    return vec3((cos(phi) * sinTheta), (sin(phi) * sinTheta), cosTheta);
}

/**
    根据探针全局索引，计算它最终在RayData中的位置
*/
uvec3 DDGIGetRayDataTexelCoords(uint rayIndex, uint probeIndex, DDGISetting volume)
{
    uint probesPerPlane = DDGIGetProbesPerPlane(volume.probeCount);
    // 在纹理中的坐标
    uvec3 coords;
    coords.x = rayIndex;
    coords.z = probeIndex / probesPerPlane;
    coords.y = probeIndex - (coords.z * probesPerPlane);
    return coords;
}
/**
 * Get the index of a probe within a horizontal plane that the probe coordinates map to, in the active coordinate system.
 */
uint DDGIGetProbeIndexInPlane(uvec3 texCoords, uvec3 probeCounts, uint probeNumTexels)
{
    return (texCoords.x / probeNumTexels) +
           probeCounts.x * uint(texCoords.y / probeNumTexels);
}

/*
  使用gl_GlobalInvocationID考虑到每个探针8*8区域后，计算探针索引，其实可以直接使用gl_WorkGroupID来计算
*/
uint DDGIGetProbeIndex(uvec3 texCoords, uint probeNumTexels, DDGISetting volume)
{
    uint probesPerPlane = DDGIGetProbesPerPlane(volume.probeCount);
    uint probeIndexInPlane = DDGIGetProbeIndexInPlane(texCoords, volume.probeCount, probeNumTexels);

    return texCoords.z * probesPerPlane + probeIndexInPlane;
}
/**
 * 获得当前线程出的6*6/14*14区域中的哪个像素，并转为UV坐标（移动到像素中心）
 0,0 对应的UV是 -1, -1
*/
vec2 DDGIGetNormalizedOctahedralCoordinates(uvec2 texCoords, uint numTexels)
{
    // 获得6*6区域中当前线程代表哪个像素 [0-5]
    vec2 octahedralTexelCoord = vec2(texCoords.x % numTexels, texCoords.y % numTexels);

    // 移动到像素中心
    octahedralTexelCoord.xy += 0.5f;

    // Normalize[0,1]
    octahedralTexelCoord.xy /= float(numTexels);

    // Shift to [-1, 1);  
    octahedralTexelCoord *= 2.f;
    octahedralTexelCoord -= vec2(1.f, 1.f);

    return octahedralTexelCoord;
}

/**
 * Returns either -1 or 1 based on the sign of the input value.
 * If the input is zero, 1 is returned.
 */
float RTXGISignNotZero(float v)
{
    return (v >= 0.f) ? 1.f : -1.f;
}

/**
 * 2-component version of RTXGISignNotZero.
 */
vec2 RTXGISignNotZero(vec2 v)
{
    return vec2(RTXGISignNotZero(v.x), RTXGISignNotZero(v.y));
}

/**
    根据6*6区域的UV坐标，计算其在八面体上的方向
 */
vec3 DDGIGetOctahedralDirection(vec2 coords)
{
    vec3 direction = vec3(coords.x, coords.y, 1.f - abs(coords.x) - abs(coords.y));
    if (direction.z < 0.f)
    {
        direction.xy = (1.f - abs(direction.yx)) * RTXGISignNotZero(direction.xy);
    }
    return normalize(direction);
}

float RGBtoLuminance(vec3 c)
{
    return dot(c, vec3(0.2125, 0.7154, 0.0721)); 
}
float RTXGIMaxComponent(vec3 a)
{
    return max(a.x, max(a.y, a.z));
}
/**
 * 对于一个世界坐标，计算一个权重
 * Computes a weight value in the range [0, 1] for a world position and volume pair.
 * All positions inside the given volume recieve a weight of 1.
 * Positions outside the volume receive a weight in [0, 1] that
 * decreases as the position moves away from the volume.
 */ 
float DDGIGetVolumeBlendWeight(vec3 worldPosition, DDGISetting volume)
{
    // Get the volume's origin and extent
    vec3 origin = volume.centerPosition;
    vec3 extent = (volume.gridStep * (volume.probeCount - 1)) * 0.5f;

    // delta就是worldPosition与volume的AABB盒的距离
    vec3 position = abs(worldPosition - origin);
    vec3 delta = position - extent;
    if(delta.x < 0.0 && delta.y < 0.0 && delta.z < 0.0) {
        return 1.0;
    }
    // Adjust the blend weight for each axis
    float volumeBlendWeight = 1.f;
    volumeBlendWeight *= (1.f - Saturate(delta.x / volume.gridStep.x));
    volumeBlendWeight *= (1.f - Saturate(delta.y / volume.gridStep.y));
    volumeBlendWeight *= (1.f - Saturate(delta.z / volume.gridStep.z));

    return volumeBlendWeight;
}

/*
    通过probeIndex和6*6区域上的UV坐标，获得最终采样需要的UVW坐标
*/
vec3 DDGIGetProbeUV(int probeIndex, vec2 octantCoordinates, int numProbeInteriorTexels, DDGISetting volume)
{
    // Get the probe's texel coordinates, assuming one texel per probe
    uvec3 coords = DDGIGetProbeCoords(probeIndex, volume);

    // Add the border texels to get the total texels per probe
    float numProbeTexels = (numProbeInteriorTexels + 2.f);

    float textureWidth = numProbeTexels * volume.probeCount.x;
    float textureHeight = numProbeTexels * volume.probeCount.z;
    // Move to the center of the probe and move to the octant texel before normalizing
    vec2 uv = vec2(coords.x * numProbeTexels, coords.z * numProbeTexels) + (numProbeTexels * 0.5f);
    uv += octantCoordinates.xy * (float(numProbeInteriorTexels) * 0.5f);
    uv /= vec2(textureWidth, textureHeight);
    return vec3(uv, coords.y);
}

/**
* 给一个方向，求对应到Octahedral的UV坐标
 * Computes the octant coordinates in the normalized [-1, 1] square, for the given a unit direction vector.
 */
vec2 DDGIGetOctahedralCoordinates(vec3 direction)
{
    float l1norm = abs(direction.x) + abs(direction.y) + abs(direction.z);
    vec2 uv = direction.xy * (1.f / l1norm);
    if (direction.z < 0.f)
    {
        uv = (1.f - abs(uv.yx)) * RTXGISignNotZero(uv.xy);
    }
    return uv;
}
// 获取离worldPosition最近的探针在探针网络的3D坐标
ivec3 DDGIGetBaseProbeGridCoords(vec3 worldPosition, DDGISetting volume){
    vec3 position = worldPosition - volume.centerPosition;
    position += (volume.gridStep * (volume.probeCount - 1)) * 0.5f;
    ivec3 probeCoords = ivec3(position / volume.gridStep);
    probeCoords = clamp(probeCoords, ivec3(0, 0, 0), (ivec3(volume.probeCount) - ivec3(1, 1, 1)));
    return probeCoords;
}


bool DDGICheckPostion(vec3 worldPosition,DDGISetting volume){
    BoundingBox box = volume.box;
    return max(min(worldPosition, box.maxBound), box.minBound) == worldPosition;
}

/*
    根据一个世界坐标，计算从探针中获取的Irrandiance
*/
vec3 DDGIGetIrrandianceByWorldPosition(vec3 worldPosition, vec3 direction, DDGISetting volume, texture2DArray IrrdianceTexture,texture2DArray distanceTexture){

    // 如果WorldPosition不在DDGIvolume内，直接返回
    if(!DDGICheckPostion(worldPosition,volume)){
        return vec3(0.f, 0.f, 0.f);
    }
    vec3  sumIrradiance = vec3(0.0f);
    float sumWeight = 0.0f;

    vec3 irradiance = vec3(0.f, 0.f, 0.f);
    float accumulatedWeights = 0.f;
    vec3 biasedWorldPosition = worldPosition;
    // 得到离worldPosition最近的探针坐标
    ivec3 baseProbeCoords = DDGIGetBaseProbeGridCoords(biasedWorldPosition, volume);
    vec3 baseProbeWorldPosition = DDGIGetProbeWorldPosition(baseProbeCoords,volume);
    vec3 alpha = clamp(((worldPosition - baseProbeWorldPosition) / volume.gridStep), vec3(0.f, 0.f, 0.f), vec3(1.f, 1.f, 1.f));

    for(int i = 0; i < 8; i++){
        float weight = 1.0;
        // 使用魔法获得一个探针😄
        ivec3  offset  = ivec3(i, i >> 1, i >> 2) & ivec3(1);
        ivec3  probeGridCoord   = clamp(baseProbeCoords + offset, ivec3(0), ivec3(volume.probeCount) - ivec3(1));
        uint probeIndex =  (probeGridCoord.y * DDGIGetProbesPerPlane(volume.probeCount)) + probeGridCoord.z * volume.probeCount.x + probeGridCoord.x ;
        vec3 probePos = DDGIGetProbeWorldPosition(probeGridCoord, volume);
        
        // 方向系数
        {
            vec3 directionToProbe = normalize(probePos - worldPosition);
            weight *= Square(max(0.0001, (dot(directionToProbe, direction) + 1.0) * 0.5)) + 0.2;  
        }

        //切比雪夫系数
        {
            vec3 probeToPoint   = worldPosition - probePos;
            vec3 dir            = normalize(-probeToPoint);
            float dist          = length(probeToPoint);
            vec2 octantCoords = DDGIGetOctahedralCoordinates(dir);
            vec3 probeTextureUV = DDGIGetProbeUV(int(probeIndex), octantCoords, int(DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR), volume);
            vec2 temp = texture(sampler2DArray(distanceTexture,SAMPLER[0]),probeTextureUV).rg;  // 采样距离纹理
            float mean      = temp.x;
            float variance  = abs(Square(temp.x) - temp.y);

            float chebyshev = variance / (variance + Square(max(dist - mean, 0.0)));
            chebyshev       = max(Pow3(chebyshev), 0.0);  //以切比雪夫系数三次方作为权重

            weight *= (dist <= mean) ? 1.0 : chebyshev;
        }

        //避免计算精度问题
        {
            weight = max(0.000001, weight); 

            const float crushThreshold = 0.2f;
            if (weight < crushThreshold)
                weight *= weight * weight * (1.0f / Square(crushThreshold)); 
        }

        //三线性插值系数
        {
            vec3 trilinear = mix(1.0 - alpha, alpha, offset);
            weight *= trilinear.x * trilinear.y * trilinear.z;  
        }

        //采样，累计光照      
        {
            vec3 irradianceDir  = direction;
            vec2 octantCoords = DDGIGetOctahedralCoordinates(direction);
            vec3 probeTextureUV = DDGIGetProbeUV(int(probeIndex), octantCoords, int(DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR), volume);
            vec3 probeIrradiance = texture(sampler2DArray(IrrdianceTexture,SAMPLER[0]),probeTextureUV).rgb;
            sumIrradiance += weight * probeIrradiance;
            sumWeight += weight;
        }

    }

    vec3 netIrradiance = sumIrradiance / sumWeight;
    netIrradiance *= 0.95;

    return 2 * PI * netIrradiance; 
}

// // 根据一个世界坐标，计算从探针中获取的Irrandiance
// vec3 DDGIGetIrrandianceByWorldPosition(vec3 worldPosition, vec3 direction,DDGISetting volume, texture2DArray IrrdianceTexture,texture2DArray distanceTexture){
//     vec3 irradiance = vec3(0.f, 0.f, 0.f);
//     float accumulatedWeights = 0.f;
//     vec3 biasedWorldPosition = worldPosition;

//     // 得到离worldPosition最近的探针坐标
//     ivec3 baseProbeCoords = DDGIGetBaseProbeGridCoords(biasedWorldPosition, volume);
//     vec3 baseProbeWorldPosition = DDGIGetProbeWorldPosition(baseProbeCoords,volume);
//     vec3 gridSpaceDistance = (worldPosition - baseProbeWorldPosition);
//     vec3 alpha = clamp((gridSpaceDistance / volume.gridStep), vec3(0.f, 0.f, 0.f), vec3(1.f, 1.f, 1.f));

//     // // 取最近的8个探针
//     for(int probeIndex = 0; probeIndex < 8; probeIndex++){
//         ivec3 adjacentProbeOffset = ivec3(probeIndex, probeIndex >> 1, probeIndex >> 2) & ivec3(1, 1, 1);
//         ivec3 adjacentProbeCoords = clamp(baseProbeCoords + adjacentProbeOffset, ivec3(0, 0, 0), ivec3(volume.probeCount) - ivec3(1, 1, 1));
//         //int adjacentProbeIndex = DDGIGetScrollingProbeIndex(adjacentProbeCoords, volume);
//         int adjacentProbeIndex = probeIndex;  // 没有上边这个功能
//         //vec3 adjacentProbeWorldPosition = DDGIGetProbeWorldPosition(adjacentProbeCoords, volume, resources.probeData); // 这里多一个参数是为了兼容Relocation功能
//         vec3 adjacentProbeWorldPosition = DDGIGetProbeWorldPosition(adjacentProbeCoords, volume);

//         vec3 worldPosToAdjProbe = normalize(adjacentProbeWorldPosition - worldPosition);
//         vec3 biasedPosToAdjProbe = normalize(adjacentProbeWorldPosition - biasedWorldPosition);
//         float  biasedPosToAdjProbeDist = length(adjacentProbeWorldPosition - biasedWorldPosition);
//         vec3 trilinear = max(vec3(0.001f), mix(1.f - alpha, alpha, adjacentProbeOffset));
//         float  trilinearWeight = (trilinear.x * trilinear.y * trilinear.z);
//         float  weight = 1.f;
//         float wrapShading = (dot(worldPosToAdjProbe, direction) + 1.f) * 0.5f;
//         weight *= (wrapShading * wrapShading) + 0.2f;
//         vec2 octantCoords = DDGIGetOctahedralCoordinates(-biasedPosToAdjProbe);
//         vec3 probeTextureUV = DDGIGetProbeUV(adjacentProbeIndex, octantCoords, int(DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR), volume);
//         // vec2 filteredDistance = 2.f * resources.probeDistance.SampleLevel(resources.bilinearSampler, probeTextureUV, 0).rg;
//         vec2 filteredDistance = texture(sampler2DArray(distanceTexture,SAMPLER[0]),probeTextureUV).rg;  // 采样纹理  TODO： 这*2，因为存的时候/2了
//         float variance = abs((filteredDistance.x * filteredDistance.x) - filteredDistance.y);

//         // 切比雪夫遮挡判断
//         float chebyshevWeight = 1.f;
//         if(biasedPosToAdjProbeDist > filteredDistance.x){
//             float v = biasedPosToAdjProbeDist - filteredDistance.x;
//             chebyshevWeight = variance / (variance + (v * v));
//             chebyshevWeight = max((chebyshevWeight * chebyshevWeight * chebyshevWeight), 0.f);
//         }
//         weight *= max(0.05f, chebyshevWeight);
//         weight = max(0.000001f, weight);
//         const float crushThreshold = 0.2f;
//         if (weight < crushThreshold)
//         {
//             weight *= (weight * weight) * (1.f / (crushThreshold * crushThreshold));
//         }
//         weight *= trilinearWeight;

//         octantCoords = DDGIGetOctahedralCoordinates(direction);
//         probeTextureUV = DDGIGetProbeUV(adjacentProbeIndex, octantCoords, int(DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR), volume);
//         vec3 probeIrradiance = texture(sampler2DArray(IrrdianceTexture,SAMPLER[0]),probeTextureUV).rgb;
//         float probeIrradianceEncodingGamma = 5.f; // TODO:volum参数 
//         vec3 exponent = vec3(probeIrradianceEncodingGamma * 0.5f);
//         probeIrradiance = pow(probeIrradiance, exponent);
//         irradiance += (weight * probeIrradiance);
//         accumulatedWeights += weight;
//     }
//     if(accumulatedWeights == 0.f) return vec3(0.f, 0.f, 0.f);
//     irradiance *= (1.f / accumulatedWeights);
//     irradiance *= irradiance;
//     irradiance *= TwoPI;

//     return irradiance;
// }
#endif