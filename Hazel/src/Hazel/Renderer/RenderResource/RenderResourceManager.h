#pragma once
#include "Hazel/Utils/IndexAllocator.h"
#include <Hazel/Renderer/RHI/RHI.h>
#include "RenderBuffer.h"
#include "Hazel/Core/Definations.h"
#include "RenderStruct.h"
#include "Sampler.h"
#include "Hazel/Scene/SceneManager.h"
#include "Texture.h"
namespace GameEngine {
    // 每帧都需要更新的资源，放在这里会自动创建多份
    struct PreFrameGlobalResources
    {

        bool isNeedUpdate = false; // 当某个实时资源更新时，其他帧也得在自己帧执行时更新好
        std::vector<RHIDescriptorUpdateInfo> updateInfos;  // 只在isNeedUpdate=true时生效


        RHIDescriptorSetRef descriptorSet;
        RenderBuffer<CameraData> cameraDataBuffer;
        RenderBuffer<LightInfo> lightInfoBuffer;
        RenderBuffer<GizmoDrawData> gizmoBuffer = RenderBuffer<GizmoDrawData>(RESOURCE_TYPE_RW_BUFFER | RESOURCE_TYPE_INDIRECT_BUFFER);
    };
    

    // 对于更新频率不高的资源，存一份即可,需要时直接Get拿
    struct MultiFrameGlobalResources
    {
        RenderBuffer<GlobalSettingInfo> globalSettingInfoBuffer;
        // 各种InfoBuffer，存储每个资源在Bindless 中的索引
        ArrayBuffer<MeshInfo, MAX_MULTI_FRAME_RESOURCE_SIZE> vertexBuffer;
        ArrayBuffer<MaterialInfo, MAX_MULTI_FRAME_RESOURCE_SIZE> materialBuffer;
        ArrayBuffer<MeshInstanceInfo, MAX_PER_FRAME_OBJECT_SIZE> meshInfoBuffer;



        // 采样器，其实Set=0里也有，这里单独创建一份Set=1
        RHIRootSignatureRef samplerRootSignature;
        RHIDescriptorSetRef samplerDescriptorSet;
        std::vector<SamplerRef> samplers;

        // 光追
        RHITopLevelAccelerationStructureRef tlas;

        // 一些有用的资源
        TextureRef whiteTexture;
        TextureRef blackTexture;
    };



    typedef struct BindlessResourceInfo
    {
        ResourceType resourceType = RESOURCE_TYPE_NONE;

        RHIBufferRef buffer;
        RHITextureViewRef textureView;
        RHISamplerRef sampler;
        uint64_t bufferOffset = 0;	// 仅buffer使用
        uint64_t bufferRange = 0;
    } BindlessResourceInfo;
	class RenderResourceManager
	{
		public:
            RenderResourceManager();
            ~RenderResourceManager() {};
            void InitPerFrameGlobalResources();
            void InitMultiFrameGlobalResources();

            void Tick();
            RHIRootSignatureRef GetSamplerRootSignature() { return m_MultiFrameGlobalResources.samplerRootSignature; }
            RHIDescriptorSetRef GetSamplerDescriptorSet() { return m_MultiFrameGlobalResources.samplerDescriptorSet; }
            RHIRootSignatureRef GetGlobalResourcePreFrameRootSignature() { return m_GlobalResourcePreFrameRootSignature; }
            RHIDescriptorSetRef GetGlobalResourcePerFrameDescriptorSet();

            // 把资源挂载到Bindless中（就是更新对应的资源描述符对应binding的index）
            uint32_t RenderResourceManager::AllocateBindlessID(const BindlessResourceInfo& resoruceInfo, BindlessSlot slot);
            void ReleaseBindlessID(uint32_t id, BindlessSlot slot);

            // Global Setting
            void SetGlobalSettingInfo(const GlobalSettingInfo& globalSettingInfo) {m_MultiFrameGlobalResources.globalSettingInfoBuffer.SetData(globalSettingInfo);};
            void SetGlobalSettingInfo() {m_MultiFrameGlobalResources.globalSettingInfoBuffer.SetData(m_GlobalSettingInfo);};

            // 材质Info
            uint32_t AllocateMaterialID() { return m_MultiFrameGlobalResources.materialBuffer.Allocate(); }
            void ReleaseMaterialID(uint32_t id) { m_MultiFrameGlobalResources.materialBuffer.Release(id); }
            void SetMaterialInfo(const MaterialInfo& materialInfo, uint32_t materialID) {m_MultiFrameGlobalResources.materialBuffer.SetData(materialInfo, materialID);};

            // 顶点Info
            uint32_t AllocateMeshInfoID() { return m_MultiFrameGlobalResources.vertexBuffer.Allocate(); }
            void ReleaseMeshInfoID(uint32_t id) { m_MultiFrameGlobalResources.vertexBuffer.Release(id); }
            void SetMeshInfo(const MeshInfo& vertexInfo, uint32_t vertexID) {m_MultiFrameGlobalResources.vertexBuffer.SetData(vertexInfo, vertexID);};

            // 实例Info
            uint32_t AllocateMeshInstanceInfoID() { return m_MultiFrameGlobalResources.meshInfoBuffer.Allocate(); }
            void ReleaseMesInstancehInfoID(uint32_t id) { m_MultiFrameGlobalResources.meshInfoBuffer.Release(id); }
            void SetMeshInstanceInfo(const MeshInstanceInfo& meshInfo, uint32_t meshID) {m_MultiFrameGlobalResources.meshInfoBuffer.SetData(meshInfo, meshID);};

            // LightInfo
            void SetLightInfo(const LightInfo& lightInfo) {m_PerFrameGlobalResources[APP_FRAMEINDEX].lightInfoBuffer.SetData(lightInfo);};
            
            // 各种Buffer数据
            void RenderResourceManager::UpdateCameraInfo();
            RenderBuffer<CameraData>& GetCameraDataBuffer() { return m_PerFrameGlobalResources[APP_FRAMEINDEX].cameraDataBuffer; }

            // Gizmo
            void SetGizmoDataCommand(void* data, int size);
            RHIBufferRef GetGizmoDataBuffer();

            // TLAS
            void SetTLAS();
            RHITopLevelAccelerationStructureRef GetTLAS() { return m_MultiFrameGlobalResources.tlas; };
            void UpdateTLAS(std::vector<RHIAccelerationStructureInstanceInfo>& instances);


    public:
        CPURenderSetting GetCPURenderSetting() { return cpuRenderSetting; };
        TextureRef GetWhiteTexture() { return m_MultiFrameGlobalResources.whiteTexture; };
        TextureRef GetBlackTexture() { return m_MultiFrameGlobalResources.blackTexture; };
    private:
        TextureRef LoadTextureFromFile(std::string filePath);
        uint32_t LoadIconFromFile(std::string filePath);
        void LoadDefaultTexture();
	private:
        // 每个飞行帧一份
        std::array<PreFrameGlobalResources, FRAMES_IN_FLIGHT> m_PerFrameGlobalResources; 
        // 全局一份
        MultiFrameGlobalResources m_MultiFrameGlobalResources; 
        // 每种类型的bindless资源，都有一个ID分配器，处理资源映射到Bindless的ID的获取和释放
		std::array<IndexAllocator, BINDLESS_SLOT_MAX_ENUM> m_BindlessIDAlloctor;
        RHIRootSignatureRef m_GlobalResourcePreFrameRootSignature; 

        // 一些资源，用于方便设置，并copy到buffer（其实可以直接定义在RenderBuffer内部）
        GlobalSettingInfo m_GlobalSettingInfo;


        CPURenderSetting cpuRenderSetting; // 从场景传递过来的一些渲染参数，只在CPU使用

    };
}

