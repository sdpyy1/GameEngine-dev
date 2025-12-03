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
#define GLORBAL_RESOURCE_BINDING_MESHINFO 18
#define GLORBAL_RESOURCE_BINDING_MATERIALINFO 19
#define GLORBAL_RESOURCE_BINDING_VERTEXINFO 20
#define GLORBAL_RESOURCE_BINDING_LIGHTINFO 21
#define GLORBAL_RESOURCE_BINDING_GIZMO 22
#define GLORBAL_RESOURCE_BINDING_TLAS 23

#ifdef RAYGEN_SHADER
    #extension GL_EXT_ray_tracing : enable
    #extension GL_EXT_ray_query : enable
    layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_TLAS) uniform accelerationStructureEXT TLAS;
#endif


layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_LIGHTINFO) readonly buffer LightInfoBuffer {

    LightInfo data;

} u_LightInfo;
layout(set = 0, binding = GLORBAL_RESOURCE_BINDING_SETTING) readonly buffer globalSettingBuffer {

    GlobalSettingInfo data;

} GLOBAL_SETTING;
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