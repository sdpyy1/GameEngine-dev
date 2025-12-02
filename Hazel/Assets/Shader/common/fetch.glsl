#ifndef FETCH_GLSL
#define FETCH_GLSL
vec4 FetchVertexPos(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0.0f);

    uint positionID = m_VertexInfo.slot[vertexID].positionID;
    if(positionID == 0) return vec4(0.0f);

    return vec4(POSITIONS[positionID].position[3 * index], 
                POSITIONS[positionID].position[3 * index + 1], 
                POSITIONS[positionID].position[3 * index + 2],
                1.0f);
}

vec3 FetchVertexNormal(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec3(0.0f, 0.0f, 1.0f);

    uint normalID = m_VertexInfo.slot[vertexID].normalID;
    if(normalID == 0) return vec3(0.0f, 0.0f, 1.0f);

    return vec3(NORMALS[normalID].normal[3 * index], 
                NORMALS[normalID].normal[3 * index + 1], 
                NORMALS[normalID].normal[3 * index + 2]);   
}

vec4 FetchVertexTangent(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0.0f);

    uint tangentID = m_VertexInfo.slot[vertexID].tangentID;
    if(tangentID == 0) return vec4(0.0f);

    return vec4(TANGENTS[tangentID].tangent[4 * index], 
                TANGENTS[tangentID].tangent[4 * index + 1], 
                TANGENTS[tangentID].tangent[4 * index + 2],
                TANGENTS[tangentID].tangent[4 * index + 3]);   
}

vec2 FetchVertexTexCoord(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec2(0.0f);

    uint texCoordID = m_VertexInfo.slot[vertexID].texCoordID;
    if(texCoordID == 0) return vec2(0.0f);

    return vec2(TEXCOORDS[texCoordID].texCoord[2 * index], 
                TEXCOORDS[texCoordID].texCoord[2 * index + 1]);
}

vec3 FetchVertexColor(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec3(0.0f);

    uint colorID = m_VertexInfo.slot[vertexID].colorID;
    if(colorID == 0) return vec3(0.0f);

    return vec3(COLORS[colorID].color[3 * index], 
                COLORS[colorID].color[3 * index + 1],
                COLORS[colorID].color[3 * index + 2]);
}

uvec4 FetchVertexBoneIndex(in uint vertexID, in uint index)
{
    if(vertexID == 0) return uvec4(0);

    uint boneIndexID = m_VertexInfo.slot[vertexID].boneIndexID;
    if(boneIndexID == 0) return uvec4(0);

    return uvec4(BONEINDEXS[boneIndexID].boneIndex[4 * index], 
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 1], 
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 2],
                 BONEINDEXS[boneIndexID].boneIndex[4 * index + 3]);   
}

vec4 FetchVertexBoneWeight(in uint vertexID, in uint index)
{
    if(vertexID == 0) return vec4(0);

    uint boneWeightID = m_VertexInfo.slot[vertexID].boneWeightID;
    if(boneWeightID == 0) return vec4(0);

    return vec4(BONEWEIGHTS[boneWeightID].boneWeight[4 * index], 
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 1], 
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 2],
                BONEWEIGHTS[boneWeightID].boneWeight[4 * index + 3]);   
}
mat4 FetchModel(in uint objectID)
{
    return u_MeshInfo.slot[objectID].model;
}

uint FetchIndex(in uint objectID, in uint offset)
{
    uint indexID = u_MeshInfo.slot[objectID].indexID;
    uint index = INDICES[indexID].index[offset];

    return index;
}
vec4 FetchPos(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return FetchVertexPos(vertexID, index);
}
vec3 FetchNormal(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return FetchVertexNormal(vertexID, index);
}
vec4 FetchTangent(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return FetchVertexTangent(vertexID, index);
}
vec3 FetchWorldNormal(in vec3 normal, in mat4 model)
{
    mat3 tbnModel = mat3(model);
    return normalize(tbnModel * normal);
}
vec4 FetchWorldTangent(in vec4 tangent, in mat4 model)
{
    mat3 tbnModel = mat3(model);
    return vec4(normalize(tbnModel * tangent.xyz), tangent.w);
}
vec2 FetchTexCoord(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return FetchVertexTexCoord(vertexID, index);
}
vec3 FetchColor(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return FetchVertexColor(vertexID, index);
}

uvec4 FetchBoneIndex(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return FetchVertexBoneIndex(vertexID, index);  
}

vec4 FetchBoneWeight(in uint objectID, in uint index)
{
    uint vertexID = u_MeshInfo.slot[objectID].vertexID;
    return FetchVertexBoneWeight(vertexID, index); 
}
vec4 FetchBaseColor(in Material material){
    return material.diffuse;  
}
vec4 FetchTex2D(in uint slot, in vec2 coord) {
	return texture(sampler2D(TEXTURES_2D[slot], SAMPLER[0]), coord);   
}

vec4 FetchTex2D(in uint slot, in vec2 coord, in float lod) {
	return textureLod(sampler2D(TEXTURES_2D[slot], SAMPLER[0]), coord, lod);   
}

vec4 FetchTexCube(in uint slot, in vec3 vector) {
	return texture(samplerCube(TEXTURES_CUBE[slot], SAMPLER[0]), vector);   
}

vec4 FetchTexCube(in uint slot, in vec3 vector, in float lod) {
	return textureLod(samplerCube(TEXTURES_CUBE[slot], SAMPLER[0]), vector, lod);   
}

vec4 FetchTex3D(in uint slot, in vec3 vector) {
	return texture(sampler3D(TEXTURES_3D[slot], SAMPLER[0]), vector);   
}

vec4 FetchTex3D(in uint slot, in vec3 vector, in float lod) {
	return textureLod(sampler3D(TEXTURES_3D[slot], SAMPLER[0]), vector, lod);   
}

Material FetchMaterial(in uint objectID) {
	return u_MaterialInfo.slot[u_MeshInfo.slot[objectID].materialID]; 
}

vec4 FetchDiffuse(in Material material, in vec2 coord) {
    if(material.textureDiffuse > 0)    
    {
        vec4 diffuse = FetchTex2D(material.textureDiffuse, coord);
        diffuse = pow(diffuse, vec4(1.0/2.2)); 
        diffuse = FetchBaseColor(material) * diffuse;         

        return diffuse;
    }
    else return FetchBaseColor(material);
}
vec4 FetchBaseEmission(in Material material){
    return material.emission;
}
vec4 FetchEmission(in Material material, in vec2 coord){
    if(material.textureEmission > 0.0){
        vec4 emission = FetchTex2D(material.textureEmission, coord);
        emission = pow(emission, vec4(1.0/2.2));     
        emission = FetchBaseEmission(material) * emission;      
        emission.w = 0;   
        return emission;
    }
     return vec4(0,0,0,0);
}

float FetchRoughness(in Material material, in vec2 coord){
    if(material.textureRoughness > 0)        
    {
        vec3 arm = FetchTex2D(material.textureRoughness, coord).xyz;
        arm = pow(arm, vec3(1.0/2.2));   
        return arm.y;
    }
    else return clamp(material.roughness, 0.00001, 0.99999); 
}
float FetchMetallic(in Material material, in vec2 coord){
    if(material.textureMetallic > 0)        
    {
        vec3 arm = FetchTex2D(material.textureMetallic, coord).xyz;
        arm = pow(arm, vec3(1.0/2.2));

        return arm.z;
    }
    else return clamp(material.metallic, 0.00001, 0.99999);   
}
vec3 FetchNormal(in Material material, in vec2 coord, in vec3 normal, in vec4 tangent)
{
    if (material.textureNormal > 0)
    {
        vec3 N = normalize(normal);
        vec3 T = normalize(tangent.xyz);
        vec3 B = normalize(cross(N, T)) * tangent.w;

        mat3 TBN = mat3(T, B, N);

        vec3 texNormal = FetchTex2D(material.textureNormal, coord).xyz;
        vec3 mapped = normalize(texNormal * 2.0 - 1.0);
    
        return normalize(TBN * mapped);
    }
    else
        return normal;
}
ShadowSetting GetShadowSetting(){
    return GLOBAL_SETTING.data.shadowSetting;
}
PostprocessInfo FetchPostprocessSetting()
{
    return GLOBAL_SETTING.data.postprocessSetting;
}
SkySetting FetchSkySetting()
{
    return GLOBAL_SETTING.data.skySetting;
}
DirLightInfo FetchDirLightInfo()
{
    return u_LightInfo.data.dirLights;
}
PointLightInfo FetchPointLightInfo(uint index)
{
    return u_LightInfo.data.pointLights[index];
}
SpotLightInfo FetchSpotLightInfo(uint index)
{
    return u_LightInfo.data.spotLights[index];
}
uint FetchPointLightCount()
{
    return u_LightInfo.data.pointLightCount;
}
uint FetchSpotLightCount()
{
    return u_LightInfo.data.spotLightCount;
}
Camera FetchCamera()
{
    return u_CameraData.data;
}





#endif // FETCH_GLSL