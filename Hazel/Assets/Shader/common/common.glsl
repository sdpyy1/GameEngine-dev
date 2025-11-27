#ifndef COMMON_GLSL
#define COMMON_GLSL
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_samplerless_texture_functions : require

#define MAX_PER_FRAME_RESOURCE_SIZE 10240			//全局的缓冲大小（个数）,包括动画，材质等
#define MAX_PER_FRAME_OBJECT_SIZE 10240			    //全局最大支持的物体数目
#define MAX_BINDLESS_RESOURCE_SIZE 10240	        //bindless 单个binding的最大描述符数目
#define MAX_POINT_LIGHT_SIZE 16
#define MAX_SPOT_LIGHT_SIZE 16
#define CSM_LEVEL_COUNT 4


struct BoundingSphere
{
    vec3 center;
    float radius;
};

struct DirLightInfo
{
    vec3 direction;
    float intensity;

    vec3 radiance;
    float _padding;

    mat4 view[CSM_LEVEL_COUNT];
    mat4 projection[CSM_LEVEL_COUNT];
    mat4 viewProj[CSM_LEVEL_COUNT];
    float SplitDepth[CSM_LEVEL_COUNT];
};

struct PointLightInfo
{
    vec3 position;
    float intensity;

    mat4 view[6];
    mat4 projection;
    mat4 viewProj[6];

    vec3 radiance;
    float _padding;

    BoundingSphere sphere; 
};

struct SpotLightInfo
{ 
    vec3 position;
    float intensity;
    float range;
    float falloff;

    mat4 view;
    mat4 projection;
    mat4 viewProj;

    vec3 radiance;
    float _padding;

    BoundingSphere sphere;
};

struct LightInfo
{
    uint dirLightCount;
    uint pointLightCount;
    uint spotLightCount;
    uint _padding0;

    DirLightInfo dirLights;
    PointLightInfo pointLights[MAX_POINT_LIGHT_SIZE];
    SpotLightInfo spotLights[MAX_SPOT_LIGHT_SIZE];
};

struct RHIIndexedIndirectCommand 
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};

struct RHIIndirectCommand 
{
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};
struct MeshInfo 
{
    mat4 model;
    // mat4 prevModel;
    //mat4 invModel;

    uint animationID;           //动画buffer索引
    uint materialID;            //材质buffer索引
    uint vertexID;
    uint indexID;  
    //uint meshCardID;            //card索引起始值
    //uint _padding[3];           

    // BoundingSphere sphere;  
   // BoundingBox box;

   // vec4 debugData;
};
struct Material 
{
    float roughness;
    float metallic;
    float alphaClip;
    uint useNormalTexture;

    vec4 baseColor;
    vec4 emission;

    uint textureDiffuse;
    uint textureNormal;
    uint textureArm;        //AO/Roughness/Metallic
    uint textureSpecular;

    int ints[8];       
    float floats[8];   

    vec4 colors[8];

    uint textureSlots2D[8]; 
    uint textureSlotsCube[4];
    uint textureSlots3D[4];  
};
struct VertexStream
{
    uint positionID;
    uint normalID;
    uint tangentID;
    uint texCoordID;
    uint colorID;
    uint boneIndexID;              
    uint boneWeightID;
    uint _padding;
};
struct Camera{
    mat4 view;
    mat4 proj;
	mat4 viewProj;
    mat4 InverseViewProj;
	float width;
	float height;
	float Near;
	float Far;
	vec3 CameraPosition;
    float padding;
};
// 全局资源绑定点
#define GLORBAL_RESOURCE_BINDING_BINDLESS_POSITION 0 
#define GLORBAL_RESOURCE_BINDING_BINDLESS_NORMAL 1
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TANGENT 2
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXCOORD 3
#define GLORBAL_RESOURCE_BINDING_BINDLESS_COLOR 4
#define GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_INDEX 5
#define GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_WEIGHT 6
#define GLORBAL_RESOURCE_BINDING_BINDLESS_ANIMATION 7
#define GLORBAL_RESOURCE_BINDING_BINDLESS_INDEX 8
        
// 采样资源
#define GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER 9
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D 10
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D_ARRAY 11
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D 12
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D_ARRAY 13
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_CUBE 14
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_3D 15

// 常规资源
#define GLORBAL_RESOURCE_BINDING_SETTING 16
#define GLORBAL_RESOURCE_BINDING_CAMERA 17
#define GLORBAL_RESOURCE_BINDING_MESHINFO 18
#define GLORBAL_RESOURCE_BINDING_MATERIALINFO 19
#define GLORBAL_RESOURCE_BINDING_VERTEXINFO 20
#define GLORBAL_RESOURCE_BINDING_LIGHTINFO 21

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_LIGHTINFO) readonly buffer LightInfoBuffer {

    LightInfo data;

} u_LightInfo;

layout(set = 0,binding = GLORBAL_RESOURCE_BINDING_CAMERA) readonly buffer CameraDataUniform{

    Camera data;

} u_CameraData;


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_MATERIALINFO) readonly buffer materials { 

    Material slot[MAX_PER_FRAME_RESOURCE_SIZE];

} u_MaterialInfo;

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_MESHINFO) readonly buffer objects {

    MeshInfo slot[MAX_PER_FRAME_OBJECT_SIZE];

} u_MeshInfo;

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_VERTEXINFO) readonly buffer vertices { 

    VertexStream slot[MAX_PER_FRAME_RESOURCE_SIZE];

} m_VertexInfo;



layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_POSITION) readonly buffer positions { 

    float position[];

} POSITIONS[MAX_BINDLESS_RESOURCE_SIZE];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_NORMAL) readonly buffer normals { 

    float normal[];

} NORMALS[MAX_BINDLESS_RESOURCE_SIZE];


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TANGENT) readonly buffer tangents { 

    float tangent[];

} TANGENTS[MAX_BINDLESS_RESOURCE_SIZE];


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXCOORD) readonly buffer texCoords { 

    float texCoord[];

} TEXCOORDS[MAX_BINDLESS_RESOURCE_SIZE];


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_COLOR) readonly buffer colors { 

    float color[];

} COLORS[MAX_BINDLESS_RESOURCE_SIZE];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_INDEX) readonly buffer boneIndexs { 

    int boneIndex[];

} BONEINDEXS[MAX_BINDLESS_RESOURCE_SIZE];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_WEIGHT) readonly buffer boneWeights { 

    float boneWeight[];

} BONEWEIGHTS[MAX_BINDLESS_RESOURCE_SIZE];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_ANIMATION) readonly buffer animations { 

    mat4 matrix[];

} ANIMATIONS[MAX_BINDLESS_RESOURCE_SIZE];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_INDEX) readonly buffer indices { 

    uint index[];

} INDICES[MAX_BINDLESS_RESOURCE_SIZE];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER) uniform sampler SAMPLER[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D) uniform texture1D TEXTURES_1D[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D_ARRAY) uniform texture1DArray TEXTURES_1D_ARRAY[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D) uniform texture2D TEXTURES_2D[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D_ARRAY) uniform texture2DArray TEXTURES_2D_ARRAY[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_CUBE) uniform textureCube TEXTURES_CUBE[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_3D) uniform texture3D TEXTURES_3D[];


// 取数据函数
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
    return material.baseColor;  
}
vec4 FetchTex2D(in uint slot, in vec2 coord) {
	return texture(sampler2D(TEXTURES_2D[slot], SAMPLER[1]), coord);   
}

vec4 FetchTex2D(in uint slot, in vec2 coord, in float lod) {
	return textureLod(sampler2D(TEXTURES_2D[slot], SAMPLER[1]), coord, lod);   
}

vec4 FetchTexCube(in uint slot, in vec3 vector) {
	return texture(samplerCube(TEXTURES_CUBE[slot], SAMPLER[1]), vector);   
}

vec4 FetchTexCube(in uint slot, in vec3 vector, in float lod) {
	return textureLod(samplerCube(TEXTURES_CUBE[slot], SAMPLER[1]), vector, lod);   
}

vec4 FetchTex3D(in uint slot, in vec3 vector) {
	return texture(sampler3D(TEXTURES_3D[slot], SAMPLER[1]), vector);   
}

vec4 FetchTex3D(in uint slot, in vec3 vector, in float lod) {
	return textureLod(sampler3D(TEXTURES_3D[slot], SAMPLER[1]), vector, lod);   
}
Material FetchMaterial(in uint objectID) {
	return u_MaterialInfo.slot[u_MeshInfo.slot[objectID].materialID]; 
}
vec4 FetchDiffuse(in Material material, in vec2 coord) {
    if(material.textureDiffuse > 0)    
    {
        vec4 diffuse = FetchTex2D(material.textureDiffuse, coord);
        diffuse = pow(diffuse, vec4(1.0/2.2));          //gamma矫正
        diffuse = FetchBaseColor(material) * diffuse;         

        return diffuse;
    }
    else return FetchBaseColor(material);
}
#endif