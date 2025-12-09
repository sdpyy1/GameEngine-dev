#ifndef DDGI_GLSL
#define DDGI_GLSL
#define DDGI_PROBE_NUM_TEXELS_IRRANDIANCE_INTERIOR 6u
#define DDGI_PROBE_NUM_TEXELS_IRRANDIANCE 8u
#define DDGI_PROBE_NUM_TEXELS_DISTANCE_INTERIOR 14u
#define DDGI_PROBE_NUM_TEXELS_DISTANCE 16u

#include "math.glsl"
uint DDGIGetProbesPerPlane(uvec3 probeCount){
    return probeCount.x * probeCount.z;
}
uvec3 DDGIGetProbeCoords(uint probeIndex, DDGISetting volume)
{
    uvec3 probeCoords;
    probeCoords.x = probeIndex % volume.probeCount.x;
    probeCoords.y = probeIndex / (volume.probeCount.x * volume.probeCount.z);
    probeCoords.z = (probeIndex / volume.probeCount.x) % volume.probeCount.z;
    return probeCoords;
}

int DDGIGetScrollingProbeIndex(vec3 probeCoords,DDGISetting volume){
    // TODO:探针面对无限场景的优化处理，还没懂
    return 1;
}

vec3 DDGIGetProbeWorldPosition(uvec3 probeCoords, DDGISetting volume){
    vec3 probeGridWorldPosition  = probeCoords * volume.gridStep;   // 网格坐标考虑探针间距后的坐标
    vec3 probeGridShift = (volume.gridStep * (volume.probeCount - 1)) * 0.5; // 偏移
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
存储结构 x: 一个探针的所有射线 y: 一层中的索引探针 z:探针层级
*/
uvec3 DDGIGetRayDataTexelCoords(uint rayIndex, uint probeIndex, DDGISetting volume)
{
    uint probesPerPlane = DDGIGetProbesPerPlane(volume.probeCount);

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
           probeCounts.x * (texCoords.y / probeNumTexels);
}

uint DDGIGetProbeIndex(uvec3 texCoords, uint probeNumTexels, DDGISetting volume)
{
    uint probesPerPlane = DDGIGetProbesPerPlane(volume.probeCount);
    uint probeIndexInPlane = DDGIGetProbeIndexInPlane(texCoords, volume.probeCount, probeNumTexels);

    return texCoords.z * probesPerPlane + probeIndexInPlane;
}
/**
 * Computes normalized octahedral coordinates for the given texel coordinates.
 * Maps the top left texel to (-1,-1).
*/
vec2 DDGIGetNormalizedOctahedralCoordinates(uvec2 texCoords, uint numTexels)
{
    // 将2D坐标转为八面体坐标
    vec2 octahedralTexelCoord = vec2(texCoords.x % numTexels, texCoords.y % numTexels);

    // 移动到像素中心
    octahedralTexelCoord.xy += 0.5f;

    // Normalize
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
 * Computes the normalized octahedral direction that corresponds to the
 * given normalized coordinates on the [-1, 1] square.
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


vec3 DDGITexIndexToNormalizedCoord(uvec3 index, DDGISetting volume) {
    vec3 textureSize = vec3(0);
    textureSize.x = volume.raysPerProbe;
    textureSize.y = DDGIGetProbesPerPlane(volume.probeCount);
    textureSize.z = volume.probeCount.y; // y_up
    vec3 pixelCenter = vec3(index) + 0.5;
    return pixelCenter / vec3(textureSize);
}
// !!! 注意采样图片需要把坐标转到[0-1]
float DDGIGetDistanceFromRayData(texture2DArray raydataTexture,uvec3 rayDataTexCoords,DDGISetting volume){
    vec3 uvw = DDGITexIndexToNormalizedCoord(rayDataTexCoords,volume);
    return texture(sampler2DArray(raydataTexture, SAMPLER[2]), uvw).w;  // 注意采样器filter是Nearest
}
// !!! 注意采样图片需要把坐标转到[0-1]
vec3 DDGIGetRandianceFromRayData(texture2DArray raydataTexture,uvec3 rayDataTexCoords,DDGISetting volume){
    vec3 uvw = DDGITexIndexToNormalizedCoord(rayDataTexCoords,volume);
    return texture(sampler2DArray(raydataTexture, SAMPLER[2]), uvw).xyz; // 注意采样器filter是Nearest
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



#endif