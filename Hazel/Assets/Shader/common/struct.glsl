#ifndef STRUCT_GLSL
#define STRUCT_GLSL

struct BoundingSphere
{
    vec3 center;
    float radius;
};
struct BoundingBox
{
    vec3 maxBound;
    float _padding0; 

    vec3 minBound;
    float _padding1;
};
struct DirectionLight
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

struct PointLight
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

struct SpotLight
{ 
    vec3 position;
    float intensity;

    float range;
    float falloff;
    uint showDirection;
    float _padding1;

    mat4 view;
    mat4 projection;
    mat4 viewProj;

    vec3 radiance;
    uint showRange;

    vec3 direction;
    float angle;

    BoundingSphere sphere;
};

struct LightInfo
{
    uint dirLightCount;
    uint pointLightCount;
    uint spotLightCount;
    uint _padding0;

    DirectionLight dirLights;
    PointLight pointLights[MAX_POINT_LIGHT_SIZE];
    SpotLight spotLights[MAX_SPOT_LIGHT_SIZE];
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

struct MeshInstanceInfo 
{
    mat4 model;
    mat4 prevModel;
    uint animationID;  
    uint materialInfoID;
    uint vertexID;
    uint indexID;  
};
struct MaterialInfo 
{
    vec4 diffuse;
    vec4 emission;

    float roughness; 
    float metallic;
    uint useNormaltexture;  
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

struct MeshInfo
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
    mat4 invView;
    mat4 invProj;
	mat4 viewProj;
    mat4 InverseViewProj;
    mat4 prevView;
    mat4 prevProj;
    mat4 projNoJetter;
	float width;
	float height;
	float Near;
	float Far;
	vec3 position;
    uint totalTick;
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
    uint _padding[2];
};
struct TAASetting
{
    uint enable;
    uint shaper;
    float shaperStrength;
    uint _padding;

    vec2 UVjetter;
	float _padding1[2];
};

struct ColorSetting {
    float exposure;
    float saturation;
    float contrast;
    uint toneMappingMode;
};
struct PathTracingSetting
{
    uint enable;
    int numSamples;
    int totalNumSamples;
    int numBounce;

    int sampleSkyBox;
    int indirectOnly;
    uint _padding[2];
};
struct BloomSetting {
    uint enable;
    float bloomScale;
    float pading[2];
};
struct PostprocessSetting {
    BloomSetting bloomSetting;
    PathTracingSetting pathTracingSetting;
    TAASetting TaaSetting;
    ColorSetting colorSetting;
};
struct SkySetting {
    uint isDynamicSky;
    float IbLScale;
    float pading[2];
};

struct DDGISetting {
    vec3 centerPosition;
    uint _padding1;

    uvec3 probeCount;
    uint enable;

    vec3 gridStep;
    uint visulaize;

    uint raysPerProbe;
    uint infineBounds;
    uint getSkyLight;
    uint _padding;

    BoundingBox box;
};
struct RenderSetting {
    uint debugDDGI;
    uint _padding[3];
};
struct GlobalSettingInfo
{ 
    RenderSetting renderSetting;
    SkySetting skySetting;
    PostprocessSetting postprocessSetting;
    ShadowSetting shadowSetting;
    IconTextureInfo iconTextures;
    DDGISetting ddgiSetting;
};
#endif