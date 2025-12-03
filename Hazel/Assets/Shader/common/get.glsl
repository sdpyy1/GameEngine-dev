#ifndef GET_GLSL
#define GET_GLSL
vec4 GetVertexPos(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0.0f);

    uint positionID = m_VertexInfo.slot[vertexID].positionID;
    if(positionID == 0) return vec4(0.0f);

    return vec4(POSITIONS[positionID].position[3 * index], 
                POSITIONS[positionID].position[3 * index + 1], 
                POSITIONS[positionID].position[3 * index + 2],
                1.0f);
}

vec3 GetVertexNormal(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec3(0.0f, 0.0f, 1.0f);

    uint normalID = m_VertexInfo.slot[vertexID].normalID;
    if(normalID == 0) return vec3(0.0f, 0.0f, 1.0f);

    return vec3(NORMALS[normalID].normal[3 * index], 
                NORMALS[normalID].normal[3 * index + 1], 
                NORMALS[normalID].normal[3 * index + 2]);   
}

vec4 GetVertexTangent(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0.0f);

    uint tangentID = m_VertexInfo.slot[vertexID].tangentID;
    if(tangentID == 0) return vec4(0.0f);

    return vec4(TANGENTS[tangentID].tangent[4 * index], 
                TANGENTS[tangentID].tangent[4 * index + 1], 
                TANGENTS[tangentID].tangent[4 * index + 2],
                TANGENTS[tangentID].tangent[4 * index + 3]);   
}

vec2 GetVertexTexCoord(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec2(0.0f);

    uint texCoordID = m_VertexInfo.slot[vertexID].texCoordID;
    if(texCoordID == 0) return vec2(0.0f);

    return vec2(TEXCOORDS[texCoordID].texCoord[2 * index], 
                TEXCOORDS[texCoordID].texCoord[2 * index + 1]);
}

vec3 GetVertexColor(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec3(0.0f);

    uint colorID = m_VertexInfo.slot[vertexID].colorID;
    if(colorID == 0) return vec3(0.0f);

    return vec3(COLORS[colorID].color[3 * index], 
                COLORS[colorID].color[3 * index + 1],
                COLORS[colorID].color[3 * index + 2]);
}

uvec4 GetVertexBoneIndex(in uint vertexID, in uint index)
{
    if(vertexID == 0) return uvec4(0);

    uint boneIndexID = m_VertexInfo.slot[vertexID].boneIndexID;
    if(boneIndexID == 0) return uvec4(0);

    return uvec4(BONEINDEXS[boneIndexID].boneIndex[4 * index], 
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 1], 
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 2],
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 3]);   
}

vec4 GetVertexBoneWeight(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0);

    uint boneWeightID = m_VertexInfo.slot[vertexID].boneWeightID;
    if(boneWeightID == 0) return vec4(0);

    return vec4(BONEWEIGHTS[boneWeightID].boneWeight[4 * index], 
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 1], 
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 2],
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 3]);   
}
mat4 GetModel(in uint objectID)
{
    return u_MeshInfo.slot[objectID].model;
}

uint GetIndex(in uint objectID, in uint offset)
{
    uint indexID = u_MeshInfo.slot[objectID].indexID;
    uint index = INDICES[indexID].index[offset];

    return index;
}
vec4 GetPos(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return GetVertexPos(vertexID, index);
}
vec3 GetNormal(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return GetVertexNormal(vertexID, index);
}
vec4 GetTangent(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
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
vec2 GetTexCoord(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return GetVertexTexCoord(vertexID, index);
}
vec3 GetColor(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return GetVertexColor(vertexID, index);
}

uvec4 GetBoneIndex(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return GetVertexBoneIndex(vertexID, index);  
}

vec4 GetBoneWeight(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return GetVertexBoneWeight(vertexID, index); 
}
vec4 GetBaseColor(in Material material){
    return material.diffuse;  
}
vec4 GetTex2D(in uint slot, in vec2 coord) {
	return texture(sampler2D(TEXTURES_2D[slot], SAMPLER[0]), coord);   
}

vec4 GetTex2D(in uint slot, in vec2 coord, in float lod) {
	return textureLod(sampler2D(TEXTURES_2D[slot], SAMPLER[0]), coord, lod);   
}

vec4 GetTexCube(in uint slot, in vec3 vector) {
	return texture(samplerCube(TEXTURES_CUBE[slot], SAMPLER[0]), vector);   
}

vec4 GetTexCube(in uint slot, in vec3 vector, in float lod) {
	return textureLod(samplerCube(TEXTURES_CUBE[slot], SAMPLER[0]), vector, lod);   
}

vec4 GetTex3D(in uint slot, in vec3 vector) {
	return texture(sampler3D(TEXTURES_3D[slot], SAMPLER[0]), vector);   
}

vec4 GetTex3D(in uint slot, in vec3 vector, in float lod) {
	return textureLod(sampler3D(TEXTURES_3D[slot], SAMPLER[0]), vector, lod);   
}

Material GetMaterial(in uint objectID) {
	return u_MaterialInfo.slot[u_MeshInfo.slot[objectID].materialID]; 
}

vec4 GetDiffuse(in Material material, in vec2 coord) {
    if(material.textureDiffuse > 0)    
    {
        vec4 diffuse = GetTex2D(material.textureDiffuse, coord);
        diffuse = pow(diffuse, vec4(1.0/2.2)); 
        diffuse = GetBaseColor(material) * diffuse;         

        return diffuse;
    }
    else return GetBaseColor(material);
}
vec4 GetBaseEmission(in Material material){
    return material.emission;
}
vec4 GetEmission(in Material material, in vec2 coord){
    if(material.textureEmission > 0.0){
        vec4 emission = GetTex2D(material.textureEmission, coord);
        emission = pow(emission, vec4(1.0/2.2));     
        emission = GetBaseEmission(material) * emission;      
        emission.w = 0;   
        return emission;
    }
     return vec4(0,0,0,0);
}

float GetRoughness(in Material material, in vec2 coord){
    if(material.textureRoughness > 0)        
    {
        vec3 arm = GetTex2D(material.textureRoughness, coord).xyz;
        arm = pow(arm, vec3(1.0/2.2));   
        return arm.y;
    }
    else return clamp(material.roughness, 0.00001, 0.99999); 
}
float GetMetallic(in Material material, in vec2 coord){
    if(material.textureMetallic > 0)        
    {
        vec3 arm = GetTex2D(material.textureMetallic, coord).xyz;
        arm = pow(arm, vec3(1.0/2.2));

        return arm.z;
    }
    else return clamp(material.metallic, 0.00001, 0.99999);   
}
vec3 GetNormal(in Material material, in vec2 coord, in vec3 normal, in vec4 tangent)
{
    if (material.textureNormal > 0)
    {
        vec3 N = normalize(normal);
        vec3 T = normalize(tangent.xyz);
        vec3 B = normalize(cross(N, T)) * tangent.w;

        mat3 TBN = mat3(T, B, N);

        vec3 texNormal = GetTex2D(material.textureNormal, coord).xyz;
        vec3 mapped = normalize(texNormal * 2.0 - 1.0);
    
        return normalize(TBN * mapped);
    }
    else
        return normal;
}
ShadowSetting GetShadowSetting(){
    return GLOBAL_SETTING.data.shadowSetting;
}
PostprocessInfo GetPostprocessSetting()
{
    return GLOBAL_SETTING.data.postprocessSetting;
}
SkySetting GetSkySetting()
{
    return GLOBAL_SETTING.data.skySetting;
}
DirLightInfo GetDirLightInfo()
{
    return u_LightInfo.data.dirLights;
}
PointLightInfo GetPointLightInfo(uint index)
{
    return u_LightInfo.data.pointLights[index];
}
SpotLightInfo GetSpotLightInfo(uint index)
{
    return u_LightInfo.data.spotLights[index];
}
uint GetPointLightCount()
{
    return u_LightInfo.data.pointLightCount;
}
uint GetSpotLightCount()
{
    return u_LightInfo.data.spotLightCount;
}
Camera GetCamera()
{
    return u_CameraData.data;
}





#endif // GET_GLSL