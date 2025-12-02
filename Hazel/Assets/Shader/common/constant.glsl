#ifndef CONSTANT_GLSL
#define CONSTANT_GLSL
// ����
const int MAX_BONES = 100;
const int MAX_ANIMATED_MESHES = 1024;
const float PI = 3.141592;
const float TwoPI = 2.0f * PI;
const float Epsilon = 0.00001;

// ���нṹ��
struct DirectionalLight
{
	vec3 Direction;
	float ShadowAmount;
	vec3 Radiance;  
	float Multiplier;
};

struct AtmosphereParameter {
	float AtmosphereHeight;
	float PlanetRadius;
	float RayleighScatteringScalarHeight;
	float MieScatteringScalarHeight;
	float OzoneLevelCenterHeight;
	float OzoneLevelWidth;
	float MieAnisotropy;
	float SeaLevel;
	vec3 SunLightColor;
	float SunLightIntensity;
	float SunDiskAngle;
};
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_samplerless_texture_functions : require

#define MAX_PER_FRAME_RESOURCE_SIZE 10240			//ȫ�ֵĻ����С��������,�������������ʵ�
#define MAX_PER_FRAME_OBJECT_SIZE 10240			    //ȫ�����֧�ֵ�������Ŀ
#define MAX_BINDLESS_RESOURCE_SIZE 10240	        //bindless ����binding�������������Ŀ
#define MAX_POINT_LIGHT_SIZE 16
#define MAX_SPOT_LIGHT_SIZE 16
#define CSM_LEVEL_COUNT 4
#define MAX_GIZMO_PRIMITIVE_COUNT 200            //gizmo���Ի��Ƶ����ͼԪ��Ŀ


struct BoundingSphere
{
    vec3 center;
    float radius;
};

struct DirLightInfo
{
    vec3 position;
    float _padding1;
    vec3 direction;
    float intensity;

    vec3 radiance;
    uint showDirection;

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
    mat4 proj;
    mat4 viewProj[6];

    vec3 radiance;
    uint showRadius;

    BoundingSphere sphere; 
};

struct SpotLightInfo
{ 
    vec3 position;
    float intensity;
    float range;
    float falloff;
    float _padding1[2];

    mat4 view;
    mat4 projection;
    mat4 viewProj;

    vec3 radiance;
    uint showRange;

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
struct GizmoBoxInfo 
{
    vec3 center;
    float _padding0;
    vec3 extent;
    float _padding1;
    vec4 color;
};

struct GizmoSphereInfo 
{
    vec3 center;
    float radious;
    vec4 color;
};

struct GizmoLineInfo 
{
    vec3 from;
    float _padding0;
    vec3 to;
    float _padding1;
    vec4 color;
};

struct GizmoBillboardInfo 
{
    vec3 center;
    uint textureID;
    vec2 extent;
    vec2 _padding;
    vec4 color;
};

struct MeshInfo 
{
    mat4 model;
    // mat4 prevModel;
    //mat4 invModel;

    uint animationID;           //����buffer����
    uint materialID;            //����buffer����
    uint vertexID;
    uint indexID;  
    //uint meshCardID;            //card������ʼֵ
    //uint _padding[3];           

    // BoundingSphere sphere;  
   // BoundingBox box;

   // vec4 debugData;
};
struct Material 
{
    vec4 diffuse;
    vec4 emission;

    float roughness; 
    float metallic;
    uint useNormaltexture;   // ��ʵֱ���ж�textureNormal��ID>0��֪����û���ˣ�����Ҫ����ֶ� Ŀǰ�պõ���_padding
    uint textureNormal;


    uint textureDiffuse;
    uint textureRoughness;
    uint textureMetallic;
    uint textureEmission;

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



struct IconTextureInfo
{
    uint dirLightID;
    uint pointLightID;
    uint spotLightID;
    uint _padding;
};

struct ShadowSetting
{
    uint DebugCSM;
    uint ShadowType;
};
struct PostprocessInfo {
    float bloomScale;
    float pading[3];
};
struct SkySetting {
    uint isDynamicSky;
    float IbLScale;
    float pading[2];
};
struct GlobalSettingInfo
{ 
    SkySetting skySetting;
    PostprocessInfo postprocessSetting;
    ShadowSetting shadowSetting;
    IconTextureInfo iconTextures;
};

#endif // CONSTANT_GLSL