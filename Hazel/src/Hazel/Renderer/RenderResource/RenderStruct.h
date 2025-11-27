#pragma once
#include <glm/ext/matrix_float4x4.hpp>
#include <Hazel/Math/AABB.h>
#define MAX_PER_FRAME_OBJECT_SIZE 10240			    //全局最大支持的物体数目
#define MAX_POINT_LIGHT_SIZE 16
#define MAX_SPOT_LIGHT_SIZE 16
#define CSM_LEVEL_COUNT 4


namespace GameEngine {
	namespace V2 {
        struct DirLightInfo
        {
            glm::vec3 direction;
            float intensity;

            glm::vec3 radiance;
            float _padding;

            glm::mat4 view[CSM_LEVEL_COUNT];
            glm::mat4 projection[CSM_LEVEL_COUNT];
            glm::mat4 viewProj[CSM_LEVEL_COUNT];
            float SplitDepth[CSM_LEVEL_COUNT];
        };

        struct PointLightInfo
        {
            glm::vec3 position;
            float intensity;

            glm::mat4 view[6];
            glm::mat4 projection;
            glm::mat4 viewProj[6];

            glm::vec3 radiance;
            float _padding;

            BoundingSphere sphere; 
        };

        struct SpotLightInfo
        { 
            glm::vec3 position;
            float intensity;
            float range;
            float falloff;

            glm::mat4 view;
            glm::mat4 projection;
            glm::mat4 viewProj;

            glm::vec3 radiance;
            float _padding;

            BoundingSphere sphere;
        };

        struct LightInfo
        {
            uint32_t dirLightCount = 0;
            uint32_t pointLightCount = 0;
            uint32_t spotLightCount = 0;
            uint32_t _padding0;

            DirLightInfo dirLights;
            PointLightInfo pointLights[MAX_POINT_LIGHT_SIZE];
            SpotLightInfo spotLights[MAX_SPOT_LIGHT_SIZE];
        };



        typedef struct VertexInfo
        {
            uint32_t positionID = 0;
            uint32_t normalID = 0;
            uint32_t tangentID = 0;
            uint32_t texCoordID = 0;
            uint32_t colorID = 0;
            uint32_t boneIndexID = 0;
            uint32_t boneWeightID = 0;
            uint32_t _padding = 0;

        } VertexInfo;

        typedef struct MeshInfo {
            glm::mat4 modelMatrix;
            uint32_t animationID;           //TODO:动画索引
            uint32_t materialID;
            uint32_t vertexID;
            uint32_t indexID;
        }MeshInfo;

        typedef struct IndirectSetting
        {
            uint32_t processSize = 0;               // 本轮需要处理的全部batch/cluster/cluster group数目
            uint32_t pipelineStateSize = 0;         // 本轮处理的不同管线状态的数目
            uint32_t _padding0[2];

            uint32_t drawSize = 0;                  // 通过culling实际需要绘制的数目，由GPU端计算写入
            uint32_t frustumCull = 0;               // 视锥剔除数目，由GPU端计算写入
            uint32_t occlusionCull = 0;             // 遮蔽剔除数目，由GPU端计算写入
            uint32_t _padding1;

        } IndirectSetting;
        typedef struct IndirectMeshDrawInfo
        {
            uint32_t objectID = 0;			        // 物体的实例索引
            uint32_t commandID = 0;				    // 使用的间接绘制指令的下标

        } IndirectMeshDrawInfo;
        
        typedef struct IndirectMeshDrawDatas        // 提交给GPU的待剔除信息
        {
            IndirectSetting setting;

            IndirectMeshDrawInfo draws[MAX_PER_FRAME_OBJECT_SIZE];

        } DrawClusterGroupDatas;

        typedef struct IndirectMeshDrawCommands     // 提交给GPU的间接绘制指令信息，被剔除的资源会置instanceCount为零；
        {                                           // 整个buffer再给mesh pass调用绘制
            RHIIndirectCommand commands[MAX_PER_FRAME_OBJECT_SIZE];

        } IndirectMeshDrawCommands;


		struct CameraData {
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 viewproj;
			float Width;
			float Height;
			float Near;
			float Far;
			glm::vec3 Position;
			float padding;
			glm::mat4 InverseViewProj;
		};


        typedef struct MaterialInfo
        {
            float roughness;
            float metallic;
            float alphaClip;
            uint32_t useNormaltexture;

            glm::vec4 diffuse;
            glm::vec4 emission;

            uint32_t textureDiffuse;
            uint32_t textureNormal; 
            uint32_t textureArm;        //AO/Roughness/Metallic
            uint32_t textureSpecular;

            //预留的通用槽位///////////////////////////////
            std::array<int32_t, 8> ints;
            std::array<float, 8> floats;
            std::array<glm::vec4, 8> colors;

            std::array<uint32_t, 8> texture2D;
            std::array<uint32_t, 4> textureCube;
            std::array<uint32_t, 4> texture3D; 

        } MaterialInfo;
	}
}