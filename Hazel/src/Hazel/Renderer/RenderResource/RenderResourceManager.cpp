#include "hzpch.h"
#include "RenderResourceManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Scene/SceneManager.h"
#define MAX_BINDLESS_RESOURCE_SIZE 10240	        //bindless 单个binding的最大描述符数目

namespace GameEngine {
	static uint32_t BindlessSlotToPerFrameBinding(BindlessSlot slot) { return slot + (uint32_t)PER_FRAME_BINDING_BINDLESS_POSITION; }

	RenderResourceManager::RenderResourceManager()
	{
		for (auto& alloctor : bindlessIDAlloctor) alloctor = IndexAllocator(MAX_BINDLESS_RESOURCE_SIZE);
		InitPerFrameGlobalResources();
	}

	void RenderResourceManager::InitPerFrameGlobalResources()
	{
		// 创建一个全局的资源描述符集来存储各种全局资源
		RHIRootSignatureInfo info = {};
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_SETTING, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_CAMERA, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, PER_FRAME_BINDING_BINDLESS_TEXTURE_2D, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE });
		m_GlobalResourceRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		for (auto& resource : m_PerFrameGlobalResources) resource.descriptorSet = m_GlobalResourceRootSignature->CreateDescriptorSet(0);
		for (auto& resource : m_PerFrameGlobalResources) {
			RHIDescriptorUpdateInfo updateInfo = {};
			updateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
            updateInfo.buffer = resource.cameraDataBuffer.GetRHIBuffer();
			updateInfo.index = 0;
			updateInfo.binding = GLORBAL_RESOURCE_BINDING_SETTING;
			resource.descriptorSet->UpdateDescriptor(updateInfo);
		}

	}
	void RenderResourceManager::ReleaseBindlessID(uint32_t id, BindlessSlot slot)
	{
		bindlessIDAlloctor[slot].Release(id);
	}
	void RenderResourceManager::Tick() // 从场景中解析数据，存入对应Buffer
	{
		m_SceneInfoFromScene = APP_SCENEMANAGER->GetSceneInfo();
		SetCameraInfo();
	}

	void RenderResourceManager::SetCameraInfo()
	{
		V2::CameraData tmpdata;
		EditorCamera& camera = m_SceneInfoFromScene.camera;
		tmpdata.view = camera.GetViewMatrix();
		tmpdata.proj = camera.GetProjectionMatrix();
		tmpdata.viewproj = camera.GetViewProjection();
		tmpdata.Width = camera.GetViewportWidth();
		tmpdata.Height = camera.GetViewportWidth();
		tmpdata.Near = camera.GetNearClip();
		tmpdata.Far = camera.GetFarClip();
		tmpdata.Position = camera.GetPosition();
		tmpdata.padding = 1.f;
		tmpdata.InverseViewProj = glm::inverse(camera.GetViewProjection());
		m_PerFrameGlobalResources[APP_FRAMEINDEX].cameraDataBuffer.SetData(tmpdata);
	}

	uint32_t RenderResourceManager::AllocateBindlessID(const BindlessResourceInfo& resoruceInfo, BindlessSlot slot)
	{
		uint32_t index = bindlessIDAlloctor[slot].Allocate();
		for (auto& resource : m_PerFrameGlobalResources)
		{
			RHIDescriptorUpdateInfo updateInfo = {};
			updateInfo.binding = BindlessSlotToPerFrameBinding(slot),
			updateInfo.index = index;
			updateInfo.resourceType = resoruceInfo.resourceType;
			updateInfo.buffer = resoruceInfo.buffer;
			updateInfo.textureView = resoruceInfo.textureView;
			updateInfo.sampler = resoruceInfo.sampler;
			updateInfo.bufferOffset = resoruceInfo.bufferOffset;
			updateInfo.bufferRange = resoruceInfo.bufferRange;
			resource.descriptorSet->UpdateDescriptor(updateInfo);
		}
		return index;
	}

	void RenderResourceManager::SetMaterialInfo(const V2::MaterialInfo& materialInfo, uint32_t materialID)
	{
		m_MultiFrameGlobalResources.materialBuffer.SetData(materialInfo, materialID);
	}
}