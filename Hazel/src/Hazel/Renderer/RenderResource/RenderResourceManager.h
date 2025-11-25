#pragma once
#include "Hazel/Utils/IndexAllocator.h"
#include <Hazel/Renderer/RHI/RHI.h>
#include <Hazel/Renderer/RHI/RHIResource.h>
#include "RenderBuffer.h"
#include "Hazel/Core/Definations.h"
#include "RenderStruct.h"
#include "Hazel/Scene/Scene.h"
#include "Sampler.h"
#define MAX_MULTI_FRAME_RESOURCE_SIZE 10240
namespace GameEngine {
    // 使用Bindless的资源
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
    enum GlobalResourceBindingID {
        // 顶点资源
        GLORBAL_RESOURCE_BINDING_BINDLESS_POSITION,
        GLORBAL_RESOURCE_BINDING_BINDLESS_NORMAL,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TANGENT,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TEXCOORD,
        GLORBAL_RESOURCE_BINDING_BINDLESS_COLOR,
        GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_INDEX,
        GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_WEIGHT,
        GLORBAL_RESOURCE_BINDING_BINDLESS_ANIMATION,
        GLORBAL_RESOURCE_BINDING_BINDLESS_INDEX,
        
        // 采样资源
        GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D_ARRAY,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D_ARRAY,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_CUBE,
        GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_3D,

        // 渲染资源
        GLORBAL_RESOURCE_BINDING_BINDLESS_MODEL_TRANSFORM,




        // 常规资源
        GLORBAL_RESOURCE_BINDING_SETTING,
        GLORBAL_RESOURCE_BINDING_CAMERA,

        GLORBAL_RESOURCE_BINDING_MAX_ENUM,//
    };

    // 每帧都需要更新的资源，每个飞行帧一份，防止冲突
    struct PreFrameGlobalResources
    {
        RHIDescriptorSetRef descriptorSet;
        RenderBuffer<V2::CameraData> cameraDataBuffer;
    };
    

    // 对于更新频率不高的资源，存一份即可,需要时直接Get拿
    struct MultiFrameGlobalResources
    {
        RHIRootSignatureRef samplerRootSignature;
        RHIDescriptorSetRef samplerDescriptorSet;   // Set=1, Binding=0 存储缓存的采样器数组
        std::vector<SamplerRef> samplers;





        // 各种InfoBuffer，存储每个资源在Bindless 中的索引
        ArrayBuffer<V2::VertexInfo, MAX_MULTI_FRAME_RESOURCE_SIZE> vertexBuffer;
        ArrayBuffer<V2::MaterialInfo, MAX_MULTI_FRAME_RESOURCE_SIZE> materialBuffer;
    };



    typedef struct BindlessResourceInfo
    {
        ResourceType resourceType = RESOURCE_TYPE_NONE;

        RHIBufferRef buffer;
        RHITextureViewRef textureView;
        RHISamplerRef sampler;
        //TODO 光追加速结构

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

            // 材质Info
            uint32_t AllocateMaterialID() { return m_MultiFrameGlobalResources.materialBuffer.Allocate(); }
            void ReleaseMaterialID(uint32_t id) { m_MultiFrameGlobalResources.materialBuffer.Release(id); }
            void SetMaterialInfo(const V2::MaterialInfo& materialInfo, uint32_t materialID) {m_MultiFrameGlobalResources.materialBuffer.SetData(materialInfo, materialID);};

            // 顶点Info
            uint32_t AllocateVertexID() { return m_MultiFrameGlobalResources.vertexBuffer.Allocate(); }
            void ReleaseVertexID(uint32_t id) { m_MultiFrameGlobalResources.vertexBuffer.Release(id); }
            void SetVertexInfo(const V2::VertexInfo& vertexInfo, uint32_t vertexID) {m_MultiFrameGlobalResources.vertexBuffer.SetData(vertexInfo, vertexID);};




            
            // 各种Buffer数据
            void RenderResourceManager::UpdateCameraInfo();
            RenderBuffer<V2::CameraData>& GetCameraDataBuffer() { return m_PerFrameGlobalResources[APP_FRAMEINDEX].cameraDataBuffer; }





	private:
        // 每个飞行帧一份
        std::array<PreFrameGlobalResources, FRAMES_IN_FLIGHT> m_PerFrameGlobalResources; 
        // 全局一份
        MultiFrameGlobalResources m_MultiFrameGlobalResources; 



        // 每种类型的bindless资源，都有一个ID分配器，处理资源映射到Bindless的ID的获取和释放
		std::array<IndexAllocator, BINDLESS_SLOT_MAX_ENUM> m_BindlessIDAlloctor;
        RHIRootSignatureRef m_GlobalResourcePreFrameRootSignature; 



        SceneInfo m_SceneInfoFromScene;
    };
}

