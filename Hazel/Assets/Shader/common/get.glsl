#ifndef GET_GLSL
#define GET_GLSL
// 取数据函数
vec4 GetVertexPos(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0.0f);

    uint positionID = MESHINFO.slot[vertexID].positionID;
    if(positionID == 0) return vec4(0.0f);

    return vec4(POSITIONS[positionID].position[3 * index], 
                POSITIONS[positionID].position[3 * index + 1], 
                POSITIONS[positionID].position[3 * index + 2],
                1.0f);
}

vec3 GetVertexNormal(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec3(0.0f, 0.0f, 1.0f);

    uint normalID = MESHINFO.slot[vertexID].normalID;
    if(normalID == 0) return vec3(0.0f, 0.0f, 1.0f);

    return vec3(NORMALS[normalID].normal[3 * index], 
                NORMALS[normalID].normal[3 * index + 1], 
                NORMALS[normalID].normal[3 * index + 2]);   
}

vec4 GetVertexTangent(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0.0f);

    uint tangentID = MESHINFO.slot[vertexID].tangentID;
    if(tangentID == 0) return vec4(0.0f);

    return vec4(TANGENTS[tangentID].tangent[4 * index], 
                TANGENTS[tangentID].tangent[4 * index + 1], 
                TANGENTS[tangentID].tangent[4 * index + 2],
                TANGENTS[tangentID].tangent[4 * index + 3]);   
}

vec2 GetVertexTexCoord(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec2(0.0f);

    uint texCoordID = MESHINFO.slot[vertexID].texCoordID;
    if(texCoordID == 0) return vec2(0.0f);

    return vec2(TEXCOORDS[texCoordID].texCoord[2 * index], 
                TEXCOORDS[texCoordID].texCoord[2 * index + 1]);
}

vec3 GetVertexColor(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec3(0.0f);

    uint colorID = MESHINFO.slot[vertexID].colorID;
    if(colorID == 0) return vec3(0.0f);

    return vec3(COLORS[colorID].color[3 * index], 
                COLORS[colorID].color[3 * index + 1],
                COLORS[colorID].color[3 * index + 2]);
}

uvec4 GetVertexBoneIndex(in uint vertexID, in uint index)
{
    if(vertexID == 0) return uvec4(0);

    uint boneIndexID = MESHINFO.slot[vertexID].boneIndexID;
    if(boneIndexID == 0) return uvec4(0);

    return uvec4(BONEINDEXS[boneIndexID].boneIndex[4 * index], 
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 1], 
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 2],
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 3]);   
}

vec4 GetVertexBoneWeight(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0);

    uint boneWeightID = MESHINFO.slot[vertexID].boneWeightID;
    if(boneWeightID == 0) return vec4(0);

    return vec4(BONEWEIGHTS[boneWeightID].boneWeight[4 * index], 
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 1], 
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 2],
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 3]);   
}
MeshInstanceInfo GetInstanceInfo(in uint instanceID){
    return MESHINSTANCEINFO.slot[instanceID];
}
mat4 GetModelMatrix(in uint instanceID)
{
    return MESHINSTANCEINFO.slot[instanceID].model;
}
mat4 GetPrevModelMatrix(in uint instanceID)
{
    return MESHINSTANCEINFO.slot[instanceID].prevModel;
}
uint GetIndex(in uint instanceID, in uint offset)
{
    uint indexID = MESHINSTANCEINFO.slot[instanceID].indexID;
    uint index = INDICES[indexID].index[offset];

    return index;
}
vec4 GetPosition(in uint instanceID, in uint index)
{
    uint vertexID = MESHINSTANCEINFO.slot[instanceID].vertexID;
    return GetVertexPos(vertexID, index);
}
vec3 GetNormal(in uint instanceID, in uint index)
{
    uint vertexID = MESHINSTANCEINFO.slot[instanceID].vertexID;
    return GetVertexNormal(vertexID, index);
}
vec4 GetTangent(in uint instanceID, in uint index)
{
    uint vertexID = MESHINSTANCEINFO.slot[instanceID].vertexID;
    return GetVertexTangent(vertexID, index);
}
vec3 GetWorldNormal(in vec3 normal, in mat4 model)
{
    mat3 tbnModel = mat3(model);
    return normalize(tbnModel * normal);
}
vec4 GetWorldTangent(in vec4 tangent, in mat4 model)
{
    mat3 tbnModel = mat3(model);
    return vec4(normalize(tbnModel * tangent.xyz), tangent.w);
}
vec2 GetTexCoord(in uint instanceID, in uint index)
{
    uint vertexID = MESHINSTANCEINFO.slot[instanceID].vertexID;
    return GetVertexTexCoord(vertexID, index);
}
vec3 GetColor(in uint instanceID, in uint index)
{
    uint vertexID = MESHINSTANCEINFO.slot[instanceID].vertexID;
    return GetVertexColor(vertexID, index);
}

uvec4 GetBoneIndex(in uint instanceID, in uint index)
{
    uint vertexID = MESHINSTANCEINFO.slot[instanceID].vertexID;
    return GetVertexBoneIndex(vertexID, index);  
}

vec4 GetBoneWeight(in uint instanceID, in uint index)
{
    uint vertexID = MESHINSTANCEINFO.slot[instanceID].vertexID;
    return GetVertexBoneWeight(vertexID, index); 
}

vec4 GetBaseColor(in MaterialInfo material){
    return material.diffuse;  
}
vec4 GetTex2D(in uint slot, in vec2 coord) {
	return texture(sampler2D(TEXTURES_2D[slot], SAMPLER[1]), coord);   
}

vec4 GetTex2D(in uint slot, in vec2 coord, in float lod) {
	return textureLod(sampler2D(TEXTURES_2D[slot], SAMPLER[1]), coord, lod);   
}

vec4 GetTexCube(in uint slot, in vec3 vector) {
	return texture(samplerCube(TEXTURES_CUBE[slot], SAMPLER[1]), vector);   
}

vec4 GetTexCube(in uint slot, in vec3 vector, in float lod) {
	return textureLod(samplerCube(TEXTURES_CUBE[slot], SAMPLER[1]), vector, lod);   
}

vec4 GetTex3D(in uint slot, in vec3 vector) {
	return texture(sampler3D(TEXTURES_3D[slot], SAMPLER[1]), vector);   
}

vec4 GetTex3D(in uint slot, in vec3 vector, in float lod) {
	return textureLod(sampler3D(TEXTURES_3D[slot], SAMPLER[1]), vector, lod);   
}

MaterialInfo GetMaterialInfo(in uint instanceID) {
	return MATERIALINFO.slot[MESHINSTANCEINFO.slot[instanceID].materialInfoID]; 
}
BoundingBox GetOriginBoundingBox(in uint instanceID) {
    return MESHINFO.slot[MESHINSTANCEINFO.slot[instanceID].vertexID].boundingBox;
}
vec4 GetDiffuse(in MaterialInfo material, in vec2 coord) {
    if(material.textureDiffuse > 0)    
    {
        vec4 diffuse = GetTex2D(material.textureDiffuse, coord);
        diffuse = GetBaseColor(material) * diffuse;         

        return diffuse;
    }
    else return GetBaseColor(material);
}
vec4 GetBaseEmission(in MaterialInfo material){
    return material.emission;
}
vec4 GetEmission(in MaterialInfo material, in vec2 coord){
    if(material.textureEmission.x > 0.0){
     vec4 emission = GetTex2D(material.textureEmission, coord);
     emission = GetBaseEmission(material) * emission;         
     return emission;
    }
    return vec4(0);
}

float GetRoughness(in MaterialInfo material, in vec2 coord){
    if(material.textureRoughness > 0)        
    {
        vec3 arm = GetTex2D(material.textureRoughness, coord).xyz * material.roughness;
        return arm.y;
    }
    else return clamp(material.roughness, 0.00001, 0.99999); 
}
float GetMetallic(in MaterialInfo material, in vec2 coord){
    if(material.textureMetallic > 0)        
    {
        vec3 arm = GetTex2D(material.textureMetallic, coord).xyz * material.metallic;
        return arm.z;
    }
    else return clamp(material.metallic, 0.00001, 0.99999);   
}
vec3 GetNormal(in MaterialInfo material, in vec2 coord, in vec3 normal, in vec4 tangent) {
	if (material.textureNormal > 0)
    {
        float fSign = tangent.w < 0 ? -1 : 1;        

        vec3 N = normalize(normal);
        vec3 T = normalize(tangent.xyz);
        vec3 B = -tangent.w * normalize(cross(N, T));
        T = fSign * normalize(cross(N, T));

        mat3 TBN = mat3(T, B, N);

        vec3 texNormal = GetTex2D(material.textureNormal, coord).xyz;
        vec3 mapped = normalize(texNormal * 2.0 - 1.0);

        return normalize(TBN * mapped);
    }
    else
        return normal;
}

////////////////////////////////////////////////////// RT  光追需要获得某个三角形三个顶点数据，手动进行插值 ////////////////////////////////////////////////////
//重心插值
float BarycentricsInterpolation(in float v0, in float v1, in float v2, vec3 barycentrics){
  return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

vec2 BarycentricsInterpolation(in vec2 v0, in vec2 v1, in vec2 v2, vec3 barycentrics){
  return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

vec3 BarycentricsInterpolation(in vec3 v0, in vec3 v1, in vec3 v2, vec3 barycentrics){
  return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

vec4 BarycentricsInterpolation(in vec4 v0, in vec4 v1, in vec4 v2, vec3 barycentrics){
  return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}


uvec3 GetTriangleIndex(in uint instanceID, in uint triangleID){
    uint indexID = MESHINSTANCEINFO.slot[instanceID].indexID;
    return uvec3(	INDICES[indexID].index[triangleID * 3],
					INDICES[indexID].index[triangleID * 3 + 1],
					INDICES[indexID].index[triangleID * 3 + 2]);
}

vec4 GetTrianglePosition(in uint instanceID, uvec3 triangleIndex, in vec3 barycentrics){
    return BarycentricsInterpolation(
		GetPosition(instanceID, triangleIndex[0]),
		GetPosition(instanceID, triangleIndex[1]),
		GetPosition(instanceID, triangleIndex[2]),
		barycentrics);
}

vec3 GetTriangleMeshNormal(in uint instanceID, uvec3 triangleIndex, in vec3 barycentrics){
        return normalize(BarycentricsInterpolation(
		GetNormal(instanceID, triangleIndex[0]),
		GetNormal(instanceID, triangleIndex[1]),
		GetNormal(instanceID, triangleIndex[2]),
		barycentrics));
}
vec4 GetTriangleTangent(in uint instanceID, in uvec3 triangleIndex, in vec3 barycentrics)
{
    return BarycentricsInterpolation(
		GetTangent(instanceID, triangleIndex[0]),
		GetTangent(instanceID, triangleIndex[1]),
		GetTangent(instanceID, triangleIndex[2]),
		barycentrics);
}
vec2 GetTriangleTexCoord(in uint instanceID, in uvec3 triangleIndex, in vec3 barycentrics)
{
    return BarycentricsInterpolation(
		GetTexCoord(instanceID, triangleIndex[0]),
		GetTexCoord(instanceID, triangleIndex[1]),
		GetTexCoord(instanceID, triangleIndex[2]),
		barycentrics);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////

ShadowSetting GetShadowSetting(){
    return GLOBAL_SETTING.data.shadowSetting;
}
PostprocessSetting GetPostprocessSetting()
{
    return GLOBAL_SETTING.data.postprocessSetting;
}
SkySetting GetSkySetting()
{
    return GLOBAL_SETTING.data.skySetting;
}
LightInfo GetLightInfo()
{
    return LIGHTINFO.data;
}
DirectionLight GetDirectionLight()
{
    return LIGHTINFO.data.dirLights;
}
PointLight GetPointLight(uint index)
{
    return LIGHTINFO.data.pointLights[index];
}
SpotLight GetSpotLight(uint index)
{
    return LIGHTINFO.data.spotLights[index];
}

ivec3 GetClusterIndex(vec3 worldPos,Camera camera)
{
    // -------------------------------
    // 1. 世界空间 → 视空间
    // -------------------------------
    vec3 viewPos = (camera.view * vec4(worldPos, 1.0)).xyz;

    // 约定：相机看向 -Z
    float zView = -viewPos.z;

    // 在近平面前或远平面外，直接 clamp
    if (zView <= camera.Near)
        zView = camera.Near;
    if (zView >= camera.Far)
        zView = camera.Far;

    // -------------------------------
    // 2. 视空间 → 裁剪空间 → NDC
    // -------------------------------
    vec4 clipPos = camera.proj * vec4(viewPos, 1.0);
    vec3 ndcPos  = clipPos.xyz / clipPos.w;   // [-1, 1]

    // -------------------------------
    // 3. NDC → 屏幕像素坐标
    // -------------------------------
    vec2 screenUV = ndcPos.xy * 0.5 + 0.5;     // [0, 1]
    vec2 pixelPos = screenUV * vec2(camera.width, camera.height);

    // -------------------------------
    // 4. 计算 cluster 尺寸
    // -------------------------------
    uint clusterX = uint((camera.width  + LIGHT_CLUSTER_GRID_SIZE - 1)
                          / LIGHT_CLUSTER_GRID_SIZE);
    uint clusterY = uint((camera.height + LIGHT_CLUSTER_GRID_SIZE - 1)
                          / LIGHT_CLUSTER_GRID_SIZE);
    uint clusterZ = LIGHT_CLUSTER_DEPTH;

    // -------------------------------
    // 5. X / Y 索引（屏幕空间）
    // -------------------------------
    uint x = uint(pixelPos.x / LIGHT_CLUSTER_GRID_SIZE);
    uint y = uint(pixelPos.y / LIGHT_CLUSTER_GRID_SIZE);

    x = clamp(x, 0u, clusterX - 1);
    y = clamp(y, 0u, clusterY - 1);

    // -------------------------------
    // 6. Z 索引（线性深度，对齐 compute）
    // -------------------------------
    float zNorm = (zView - camera.Near) / (camera.Far - camera.Near);
    uint  z     = uint(zNorm * float(clusterZ));

    z = clamp(z, 0u, clusterZ - 1);

    return ivec3(x, y, z);
}


uint GetPointLightCount()
{
    return LIGHTINFO.data.pointLightCount;
}
uint GetSpotLightCount()
{
    return LIGHTINFO.data.spotLightCount;
}
Camera GetCamera()
{
    return CAMERAINFO.data;
}

RenderSetting GetRenderSetting()
{
    return GLOBAL_SETTING.data.renderSetting;
}
DDGISetting GetDDGISetting()
{
    return GLOBAL_SETTING.data.ddgiSetting;
}
BloomSetting GetBloomSetting()
{
    return GLOBAL_SETTING.data.postprocessSetting.bloomSetting;
}
PathTracingSetting GetPathTracingSetting()
{
    return GLOBAL_SETTING.data.postprocessSetting.pathTracingSetting;
}
TAASetting GetTAASetting()
{
    return GLOBAL_SETTING.data.postprocessSetting.TaaSetting;
}
ColorSetting GetColorSetting()
{
    return GLOBAL_SETTING.data.postprocessSetting.colorSetting;
}
#endif // GET_GLSL