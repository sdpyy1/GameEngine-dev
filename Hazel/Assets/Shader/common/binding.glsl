#define GLORBAL_RESOURCE_BINDING_BINDLESS_POSITION 0 
#define GLORBAL_RESOURCE_BINDING_BINDLESS_NORMAL 1
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TANGENT 2
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXCOORD 3
#define GLORBAL_RESOURCE_BINDING_BINDLESS_COLOR 4
#define GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_INDEX 5
#define GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_WEIGHT 6
#define GLORBAL_RESOURCE_BINDING_BINDLESS_ANIMATION 7
#define GLORBAL_RESOURCE_BINDING_BINDLESS_INDEX 8
        
#define GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER 9
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D 10
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D_ARRAY 11
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D 12
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D_ARRAY 13
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_CUBE 14
#define GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_3D 15

#define GLORBAL_RESOURCE_BINDING_SETTING 16
#define GLORBAL_RESOURCE_BINDING_CAMERA 17
#define GLORBAL_RESOURCE_BINDING_MESHINSTANCEINFO 18
#define GLORBAL_RESOURCE_BINDING_MATERIALINFO 19
#define GLORBAL_RESOURCE_BINDING_MESHINFO 20
#define GLORBAL_RESOURCE_BINDING_LIGHTINFO 21
#define GLORBAL_RESOURCE_BINDING_GIZMO 22
#define GLORBAL_RESOURCE_BINDING_TLAS 23

#ifdef RAYGEN_SHADER
    #extension GL_EXT_ray_tracing : enable
    #extension GL_EXT_ray_query : enable
    layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_TLAS) uniform accelerationStructureEXT TLAS;
#endif


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_LIGHTINFO) buffer LightInfoBuffer {

    LightInfo data;

} LIGHTINFO;
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_SETTING) readonly buffer globalSettingBuffer {

    GlobalSettingInfo data;

} GLOBAL_SETTING;
layout(set = 0,binding = GLORBAL_RESOURCE_BINDING_CAMERA) readonly buffer CameraDataUniform{

    Camera data;

} CAMERAINFO;


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_MATERIALINFO) readonly buffer MaterialInfos { 

    MaterialInfo slot[MAX_PER_FRAME_RESOURCE_SIZE];

} MATERIALINFO;

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_MESHINSTANCEINFO) readonly buffer instanceInfo {

    MeshInstanceInfo slot[MAX_PER_FRAME_INSTANCE_SIZE];

} MESHINSTANCEINFO;

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_MESHINFO) readonly buffer vertices { 

    MeshInfo slot[MAX_PER_FRAME_RESOURCE_SIZE];

} MESHINFO;

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_GIZMO) buffer gizmoDrawData 
{ 
    RHIIndexedIndirectCommand command[4];
    GizmoBoxInfo boxes[MAX_GIZMO_PRIMITIVE_COUNT];
    GizmoSphereInfo spheres[MAX_GIZMO_PRIMITIVE_COUNT];
    GizmoLineInfo lines[MAX_GIZMO_PRIMITIVE_COUNT];
    GizmoBillboardInfo worldBillboards[MAX_GIZMO_PRIMITIVE_COUNT];

} GIZMO_DRAW_DATA;

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_POSITION) readonly buffer positions { 

    float position[];

} POSITIONS[];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_NORMAL) readonly buffer normals { 

    float normal[];

} NORMALS[];


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TANGENT) readonly buffer tangents { 

    float tangent[];

} TANGENTS[];


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXCOORD) readonly buffer texCoords { 

    float texCoord[];

} TEXCOORDS[];


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_COLOR) readonly buffer colors { 

    float color[];

} COLORS[];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_INDEX) readonly buffer boneIndexs { 

    int boneIndex[];

} BONEINDEXS[];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_WEIGHT) readonly buffer boneWeights { 

    float boneWeight[];

} BONEWEIGHTS[];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_ANIMATION) readonly buffer animations { 

    mat4 matrix[];

} ANIMATIONS[];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_INDEX) readonly buffer indices { 

    uint index[];

} INDICES[];

layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER) uniform sampler SAMPLER[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D) uniform texture1D TEXTURES_1D[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D_ARRAY) uniform texture1DArray TEXTURES_1D_ARRAY[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D) uniform texture2D TEXTURES_2D[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D_ARRAY) uniform texture2DArray TEXTURES_2D_ARRAY[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_CUBE) uniform textureCube TEXTURES_CUBE[];
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_3D) uniform texture3D TEXTURES_3D[];