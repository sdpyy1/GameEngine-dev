#pragma once
#include "Hazel/Utils/IndexAllocator.h"
#include <Hazel/Renderer/RHI/RHI.h>
#include <Hazel/Renderer/RHI/RHIResource.h>
#include "RenderBuffer.h"
#include "Hazel/Core/Definations.h"
#include "RenderStruct.h"
#include "Hazel/Scene/Scene.h"
#define MAX_PER_FRAME_RESOURCE_SIZE 10240
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
        GLORBAL_RESOURCE_BINDING_SETTING = 0,
        GLORBAL_RESOURCE_BINDING_CAMERA,


        PER_FRAME_BINDING_BINDLESS_POSITION,
        PER_FRAME_BINDING_BINDLESS_NORMAL,
        PER_FRAME_BINDING_BINDLESS_TANGENT,
        PER_FRAME_BINDING_BINDLESS_TEXCOORD,
        PER_FRAME_BINDING_BINDLESS_COLOR,
        PER_FRAME_BINDING_BINDLESS_BONE_INDEX,
        PER_FRAME_BINDING_BINDLESS_BONE_WEIGHT,
        PER_FRAME_BINDING_BINDLESS_ANIMATION,
        PER_FRAME_BINDING_BINDLESS_INDEX,

        PER_FRAME_BINDING_BINDLESS_SAMPLER,
        PER_FRAME_BINDING_BINDLESS_TEXTURE_1D,
        PER_FRAME_BINDING_BINDLESS_TEXTURE_1D_ARRAY,
        PER_FRAME_BINDING_BINDLESS_TEXTURE_2D,
        PER_FRAME_BINDING_BINDLESS_TEXTURE_2D_ARRAY,
        PER_FRAME_BINDING_BINDLESS_TEXTURE_CUBE,
        PER_FRAME_BINDING_BINDLESS_TEXTURE_3D,
        PER_FRAME_BINDING_MAX_ENUM,//
    };

    // 每帧都需要更新的资源，每个飞行帧一份，防止冲突
    struct PreFrameGlobalResources
    {
        RHIDescriptorSetRef descriptorSet;
        RenderBuffer<V2::CameraData> cameraDataBuffer;
    };

    struct MultiFrameGlobalResources
    {
        ArrayBuffer<V2::MaterialInfo, MAX_PER_FRAME_RESOURCE_SIZE> materialBuffer;
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
            void Tick();

            // ID
            uint32_t RenderResourceManager::AllocateBindlessID(const BindlessResourceInfo& resoruceInfo, BindlessSlot slot);
            void ReleaseBindlessID(uint32_t id, BindlessSlot slot);

            // 材质
            uint32_t AllocateMaterialID() { return m_MultiFrameGlobalResources.materialBuffer.Allocate(); }
            void ReleaseMaterialID(uint32_t id) { m_MultiFrameGlobalResources.materialBuffer.Release(id); }
            void SetMaterialInfo(const V2::MaterialInfo& materialInfo, uint32_t materialID);




            // 各种Buffer数据
            void RenderResourceManager::SetCameraInfo();
            RenderBuffer<V2::CameraData>& GetCameraDataBuffer() { return m_PerFrameGlobalResources[APP_FRAMEINDEX].cameraDataBuffer; }






	private:
        // 每个飞行帧一份
        std::array<PreFrameGlobalResources, FRAMES_IN_FLIGHT> m_PerFrameGlobalResources; 
        // 全局一份
        MultiFrameGlobalResources m_MultiFrameGlobalResources; 



        // 每种资源一个ID分配器
		std::array<IndexAllocator, BINDLESS_SLOT_MAX_ENUM> bindlessIDAlloctor;
        RHIRootSignatureRef m_GlobalResourceRootSignature; 



        SceneInfo m_SceneInfoFromScene;
	};
}

