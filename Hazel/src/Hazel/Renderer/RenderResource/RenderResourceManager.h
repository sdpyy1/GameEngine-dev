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
		// 默认摄像机来自SceneManager自带，激活摄像机可以来自摄像机组件或者默认摄像机
		RenderBuffer<CameraData> activeCameraDataBuffer;
		RenderBuffer<CameraData> defalutCameraDataBuffer;
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
		ArrayBuffer<MeshInstanceInfo, MAX_PER_FRAME_INSTANCE_SIZE> meshInfoBuffer;

		// 采样器，其实Set=0里也有，这里单独创建一份Set=1
		RHIRootSignatureRef samplerRootSignature;
		RHIDescriptorSetRef samplerDescriptorSet;
		std::vector<SamplerRef> samplers;

		// 一些有用的资源
		TextureRef whiteTexture;
		TextureRef blackTexture;

		// 非一帧资源
		RHITextureRef HZB;
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
		RHITextureRef GetHZB() { return m_MultiFrameGlobalResources.HZB; };
		// 把资源挂载到Bindless中（就是更新对应的资源描述符对应binding的index）
		uint32_t RenderResourceManager::AllocateBindlessID(const BindlessResourceInfo& resoruceInfo, BindlessSlot slot);
		void ReleaseBindlessID(uint32_t id, BindlessSlot slot);

		// Global Setting
		void SetGlobalSettingInfo(const GlobalSettingInfo& globalSettingInfo) { m_MultiFrameGlobalResources.globalSettingInfoBuffer.SetData(globalSettingInfo); };
		void SetGlobalSettingInfo() { m_MultiFrameGlobalResources.globalSettingInfoBuffer.SetData(m_GlobalSettingInfo); };
		GlobalSettingInfo GetGlobalSettingInfo() { return m_GlobalSettingInfo; };

		// 材质Info
		uint32_t AllocateMaterialID() { return m_MultiFrameGlobalResources.materialBuffer.Allocate(); }
		void ReleaseMaterialID(uint32_t id) { m_MultiFrameGlobalResources.materialBuffer.Release(id); }
		void SetMaterialInfo(const MaterialInfo& materialInfo, uint32_t materialID) { m_MultiFrameGlobalResources.materialBuffer.SetData(materialInfo, materialID); };

		// 顶点Info
		uint32_t AllocateMeshInfoID() { return m_MultiFrameGlobalResources.vertexBuffer.Allocate(); }
		void ReleaseMeshInfoID(uint32_t id) { m_MultiFrameGlobalResources.vertexBuffer.Release(id); }
		void SetMeshInfo(const MeshInfo& vertexInfo, uint32_t vertexID) { m_MultiFrameGlobalResources.vertexBuffer.SetData(vertexInfo, vertexID); };

		// 实例Info
		uint32_t AllocateMeshInstanceInfoID() { return m_MultiFrameGlobalResources.meshInfoBuffer.Allocate(); }
		void ReleaseMesInstancehInfoID(uint32_t id) { m_MultiFrameGlobalResources.meshInfoBuffer.Release(id); }
		void SetMeshInstanceInfo(const MeshInstanceInfo& meshInfo, uint32_t meshID) { m_MultiFrameGlobalResources.meshInfoBuffer.SetData(meshInfo, meshID); };
		void SetMeshInstanceInfoBatch(const std::vector<MeshInstanceInfo>& meshInfos) { m_MultiFrameGlobalResources.meshInfoBuffer.SetData(meshInfos); };

		// LightInfo
		void SetLightInfo(const LightInfo& lightInfo) { m_PerFrameGlobalResources[APP_FRAMEINDEX].lightInfoBuffer.SetData(lightInfo); };

		// 各种Buffer数据
		void RenderResourceManager::UpdateCameraInfo();
		RenderBuffer<CameraData>& GetCameraDataBuffer() { return m_PerFrameGlobalResources[APP_FRAMEINDEX].activeCameraDataBuffer; }

		// Gizmo
		void SetGizmoDataCommand(void* data, int size);
		RHIBufferRef GetGizmoDataBuffer();

		// TLAS（单一共享资源，所有飞行帧的 descriptor set 都绑定同一份）
		void SetTLAS();
		RHITopLevelAccelerationStructureRef GetTLAS() { return m_SharedTLAS; };
		void UpdateTLAS(std::vector<RHIAccelerationStructureInstanceInfo>& instances);

	public:
		CPURenderSetting GetCPURenderSetting() { return cpuRenderSetting; };
		TextureRef GetWhiteTexture() { return m_MultiFrameGlobalResources.whiteTexture; };
		TextureRef GetBlackTexture() { return m_MultiFrameGlobalResources.blackTexture; };
	private:
		TextureRef LoadTextureFromFile(std::string filePath);
		uint32_t LoadIconFromFile(std::string filePath);
		CameraData BuildCameraUpdateData(EditorCameraRef camera);

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

		// 记录上次构建 TLAS 时的场景版本；场景切换后版本变化，需销毁旧 TLAS 强制重建
		uint32_t m_LastBuiltSceneVersion = UINT32_MAX;
		// 记录上次构建时的实例数量。TLAS 的 UPDATE 模式要求 primitiveCount 与首次 BUILD 时一致，
		// 因此实例数变化时也必须走 BUILD（销毁重建）而非 UPDATE，否则会违反规范导致 VK_ERROR_DEVICE_LOST。
		uint32_t m_LastBuiltInstanceCount = UINT32_MAX;
		// 标记 TLAS 需要重建（场景切换或实例数变化时置位，真正重建前会先等待 GPU 空闲，避免销毁正在被 in-flight 帧引用的加速结构）
		bool m_NeedRebuildTLAS = false;
		// 光追顶层加速结构，作为单一共享资源（不随飞行帧复制，避免部分帧 descriptor set 悬空）
		RHITopLevelAccelerationStructureRef m_SharedTLAS;
	};
}
