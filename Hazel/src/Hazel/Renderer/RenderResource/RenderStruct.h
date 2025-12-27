#pragma once
#include <glm/ext/matrix_float4x4.hpp>
#include <Hazel/Math/collision.h>

#define MAX_MULTI_FRAME_RESOURCE_SIZE 10240
#define MAX_BINDLESS_RESOURCE_SIZE 10240	        //bindless 单个binding的最大描述符数目
#define MAX_PER_FRAME_INSTANCE_SIZE 10240			//全局最大支持的物体数目
#define MAX_GIZMO_PRIMITIVE_COUNT 102400				// Gizmo
#define MAX_PER_PASS_PIPELINE_STATE_COUNT 1024      // pipeline缓存数量


#define LIGHT_CLUSTER_DEPTH 128						// 视锥分簇深度划分数
#define LIGHT_CLUSTER_GRID_SIZE 64					// UV方向划分粒度
#define MAX_LIGHTS_PER_CLUSTER 8					// 每个簇最多支持的灯光数




#define MAX_POINT_LIGHT_SIZE 16
#define MAX_SPOT_LIGHT_SIZE 16
#define CSM_LEVEL_COUNT 4

namespace GameEngine {
	// 使用Bindless的资源，每个Slot都是Set=0的资源描述符的一个binding，binding绑定的是一个无界数组
	enum BindlessSlot
	{
		BINDLESS_SLOT_POSITION = 0,
		BINDLESS_SLOT_NORMAL,
		BINDLESS_SLOT_TANGENT,
		BINDLESS_SLOT_TEXCOORD,
		BINDLESS_SLOT_COLOR,
		BINDLESS_SLOT_BONE_INDEX,
		BINDLESS_SLOT_BONE_WEIGHT,
		BINDLESS_SLOT_ANIMATION,
		BINDLESS_SLOT_INDEX,

		BINDLESS_SLOT_SAMPLER,
		BINDLESS_SLOT_TEXTURE_1D,
		BINDLESS_SLOT_TEXTURE_1D_ARRAY,
		BINDLESS_SLOT_TEXTURE_2D,
		BINDLESS_SLOT_TEXTURE_2D_ARRAY,
		BINDLESS_SLOT_TEXTURE_CUBE,
		BINDLESS_SLOT_TEXTURE_3D,
		BINDLESS_SLOT_MAX_ENUM,     //
	};

// G-Buffer 资源绑定点  注意Set = 2
#define GBUFFER_POSITION_BINDING 0
#define GBUFFER_NORMAL_BINDING 1
#define GBUFFER_MATERIAL_BINDING 2
#define GBUFFER_ALBEDO_BINDING 3

// Bindless绑定点
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
#define GLORBAL_RESOURCE_BINDING_MESHINSTANCEINFO 18
#define GLORBAL_RESOURCE_BINDING_MATERIALINFO 19
#define GLORBAL_RESOURCE_BINDING_MESHINFO 20
#define GLORBAL_RESOURCE_BINDING_LIGHTINFO 21
#define GLORBAL_RESOURCE_BINDING_GIZMO 22
#define GLORBAL_RESOURCE_BINDING_TLAS 23

	// 每个簇存储的信息
	struct LightingClusterInfo {
		uint32_t lightCount;
		uint32_t firstLightIndex;
	};
	// 所有簇的光源信息
	struct LightingClusterIdInfo {
		uint32_t lightID;
	};



	struct DirectionLight
	{
		glm::vec3 position;
		float _padding1;

		glm::vec3 direction;
		float intensity;

		glm::vec3 radiance;
		uint32_t showDirection;

		glm::mat4 view[CSM_LEVEL_COUNT];
		glm::mat4 projection[CSM_LEVEL_COUNT];
		glm::mat4 viewProj[CSM_LEVEL_COUNT];
		Frustum frustum[CSM_LEVEL_COUNT];

		float SplitDepth[CSM_LEVEL_COUNT];
	};

	struct PointLight
	{
		glm::vec3 position;
		float intensity;

		glm::mat4 view[6];
		glm::mat4 projection;
		glm::mat4 viewProj[6];

		glm::vec3 radiance;
		uint32_t showRadius;

		BoundingSphere sphere;
	};

	struct SpotLight
	{
		glm::vec3 position;
		float intensity;

		float range;
		float falloff;
		uint32_t showDirection;
		float _padding1;

		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 viewProj;

		glm::vec3 radiance;
		uint32_t showRange;

		glm::vec3 direction;
		float angle;


		BoundingSphere sphere;
	};

	struct LightInfo
	{
		uint32_t directionLightCount = 0;
		uint32_t pointLightCount = 0;
		uint32_t spotLightCount = 0;
		uint32_t clusterAtomicOffset;   // ClusterLighting统计时使用

		DirectionLight dirLights;
		PointLight pointLights[MAX_POINT_LIGHT_SIZE];
		SpotLight spotLights[MAX_SPOT_LIGHT_SIZE];
	};

	// 读取的Mesh信息
	typedef struct MeshInfo
	{
		uint32_t positionID = 0;
		uint32_t normalID = 0;
		uint32_t tangentID = 0;
		uint32_t texCoordID = 0;

		uint32_t colorID = 0;
		uint32_t boneIndexID = 0;
		uint32_t boneWeightID = 0;
		uint32_t _padding = 0;

		BoundingBox box;
	} MeshInfo;

	// Mesh的实例信息
	typedef struct MeshInstanceInfo {
		glm::mat4 modelMatrix;
		glm::mat4 prevModelMatrix;
		uint32_t animationID;           //TODO:动画索引
		uint32_t materialID;
		uint32_t vertexID;
		uint32_t indexID;
	}MeshInstanceInfo;


	struct CameraData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 invView;
		glm::mat4 invProj;
		glm::mat4 viewproj;
		glm::mat4 invPV;
		glm::mat4 prevView;
		glm::mat4 prevProj;
		glm::mat4 projNoJetter;

		float Width;
		float Height;
		float Near;
		float Far;

		glm::vec3 Position;
		uint32_t totalTick;

		glm::vec3 forward;
		uint32_t _padding;

		Frustum frustum;
	};

	typedef struct MaterialInfo
	{
		glm::vec4 diffuse;
		glm::vec4 emission;

		float roughness;
		float metallic;
		uint32_t useNormaltexture;
		uint32_t textureNormal;

		uint32_t textureDiffuse;
		uint32_t textureRoughness;
		uint32_t textureMetallic;
		uint32_t textureEmission;

		//预留的通用槽位///////////////////////////////
		std::array<int32_t, 8> ints;
		std::array<float, 8> floats;
		std::array<glm::vec4, 8> colors;

		std::array<uint32_t, 8> texture2D;
		std::array<uint32_t, 4> textureCube;
		std::array<uint32_t, 4> texture3D;
	} MaterialInfo;

	typedef struct GizmoBoxInfo
	{
		glm::vec3 center;
		float _padding0;
		glm::vec3 extent;
		float _padding1;
		glm::vec4 color;
	} GizmoBoxInfo;

	typedef struct GizmoSphereInfo
	{
		glm::vec3 center;
		float radious;
		glm::vec4 color;
	} GizmoSphereInfo;

	typedef struct GizmoLineInfo
	{
		glm::vec3 from;
		float _padding0;
		glm::vec3 to;
		float _padding1;
		glm::vec4 color;
	} GizmoLineInfo;

	struct GizmoBillboardInfo
	{
		glm::vec3 center;
		uint32_t textureID;
		glm::vec2 extent;
		glm::vec2 _padding;
		glm::vec4 color;
	};

	struct GizmoDrawData {
		RHIIndexedIndirectCommand command[4];  // 用于给GPU传递间接渲染指令

		GizmoBoxInfo boxes[MAX_GIZMO_PRIMITIVE_COUNT];
		GizmoSphereInfo spheres[MAX_GIZMO_PRIMITIVE_COUNT];
		GizmoLineInfo lines[MAX_GIZMO_PRIMITIVE_COUNT];
		GizmoBillboardInfo worldBillboards[MAX_GIZMO_PRIMITIVE_COUNT];
	};

	struct IconTextureInfo
	{
		uint32_t dirLightID;
		uint32_t pointLightID;
		uint32_t spotLightID;
		uint32_t cameraLightID;
	};
	enum ShadowType : uint32_t
	{
		SHADOW_TYPE_NONE = 0,
		SHADOW_TYPE_HARD,
		SHADOW_TYPE_PCF,
		SHADOW_TYPE_PCSS,
		SHADOW_TYPE_VSM
	};
	struct ShadowSetting
	{
		uint32_t DebugCSM;
		uint32_t ShadowType = SHADOW_TYPE_PCSS;
        uint32_t _padding[2];
	};

	struct TAASetting
	{
		uint32_t enable;
		uint32_t shaper;
		float shaperStrength;
		uint32_t _padding;

		glm::vec2 UVjetter;
		float _padding1[2];
	};
	struct ColorSetting {
		float exposure = 0.4;
		float saturation = 1.0;
		float contrast = 1.0;
		uint32_t toneMappingMode = 0;
	};
	struct PathTracingSetting
	{
		uint32_t enable;
		uint32_t numSamples;
		uint32_t totalNumSamples;
		uint32_t numBounce;

		uint32_t sampleSkyBox;
		uint32_t indirectOnly;
		uint32_t historyActive;
		uint32_t _padding[1];
	};
	struct BloomSetting {
		uint32_t enable;
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
		uint32_t isDynamicSky;
		float IbLScale;
		float pading[2];
	};
	struct DDGISetting {
		glm::vec3 centerPosition;
		uint32_t _padding1;

		glm::uvec3 probeCount;
		uint32_t enable;

		glm::vec3 gridStep;
		uint32_t visulaize;

		uint32_t raysPerProbe;
		uint32_t infineBounds;
		uint32_t getSkyLight;
		uint32_t _padding;

		BoundingBox box;
	};
	struct RenderSettingInfo {
		uint32_t debugDDGI;
		uint32_t renderBoundingBox;
		uint32_t ClusterLightFrustumDebug;
		uint32_t _padding[1];
	};


	struct GlobalSettingInfo
	{
		RenderSettingInfo renderSetting;
		SkySetting skySetting;
		PostprocessSetting postprocess;
		ShadowSetting shadowSetting;
		IconTextureInfo iconTextures;
		DDGISetting ddgiSetting;

	};
}