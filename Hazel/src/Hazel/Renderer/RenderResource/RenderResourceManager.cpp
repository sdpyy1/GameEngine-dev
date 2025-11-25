#include "hzpch.h"
#include "RenderResourceManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Scene/SceneManager.h"
#define MAX_BINDLESS_RESOURCE_SIZE 10240	        //bindless 单个binding的最大描述符数目

namespace GameEngine {
	static uint32_t BindlessSlotToPerFrameBinding(BindlessSlot slot) { return slot + (uint32_t)GLORBAL_RESOURCE_BINDING_BINDLESS_POSITION; }

	RenderResourceManager::RenderResourceManager()
	{
		for (auto& alloctor : m_BindlessIDAlloctor) alloctor = IndexAllocator(MAX_BINDLESS_RESOURCE_SIZE);
		InitPerFrameGlobalResources();
		InitMultiFrameGlobalResources();
	}

	void RenderResourceManager::InitPerFrameGlobalResources()
	{
		// 创建一个全局的资源描述符集来挂载各种全局资源
		RHIRootSignatureInfo info = {};
		// set binding count frequency type
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_POSITION, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_NORMAL, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TANGENT, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXCOORD, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_COLOR, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_INDEX, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_BONE_WEIGHT, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_ANIMATION, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_INDEX, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });

		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_SAMPLER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_1D_ARRAY, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_2D_ARRAY, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_CUBE, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE_CUBE });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_3D, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE });


		// 下面这些需要手动去绑定buffer和preFrame资源描述符
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_MESHINFO, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_MATERIALINFO, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_VERTEXINFO, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });

		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_SETTING, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_CAMERA, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_BUFFER });


		m_GlobalResourcePreFrameRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		for (auto& resource : m_PerFrameGlobalResources) resource.descriptorSet = m_GlobalResourcePreFrameRootSignature->CreateDescriptorSet(0);

		// 挂载默认全局资源
		for (auto& resource : m_PerFrameGlobalResources) {
			// camera
			RHIDescriptorUpdateInfo cameraUpdateInfo = {};
			cameraUpdateInfo.resourceType = RESOURCE_TYPE_BUFFER;
			cameraUpdateInfo.buffer = resource.cameraDataBuffer.GetRHIBuffer();
			cameraUpdateInfo.index = 0;
			cameraUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_CAMERA;
			resource.descriptorSet->UpdateDescriptor(cameraUpdateInfo);
			
			// Setting


			// meshInfo
			RHIDescriptorUpdateInfo meshInfoUpdateInfo = {};
            meshInfoUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
            meshInfoUpdateInfo.buffer = m_MultiFrameGlobalResources.meshInfoBuffer.GetRHIBuffer();
			cameraUpdateInfo.index = 0;
			cameraUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_MESHINFO;
			resource.descriptorSet->UpdateDescriptor(meshInfoUpdateInfo);

			// materialInfo
            RHIDescriptorUpdateInfo materialInfoUpdateInfo = {};
            materialInfoUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
            materialInfoUpdateInfo.buffer = m_MultiFrameGlobalResources.materialBuffer.GetRHIBuffer();
            materialInfoUpdateInfo.index = 0;
            materialInfoUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_MATERIALINFO;
            resource.descriptorSet->UpdateDescriptor(materialInfoUpdateInfo);

			// vertexInfo
            RHIDescriptorUpdateInfo vertexInfoUpdateInfo = {};
            vertexInfoUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
            vertexInfoUpdateInfo.buffer = m_MultiFrameGlobalResources.vertexBuffer.GetRHIBuffer();
            vertexInfoUpdateInfo.index = 0;
            vertexInfoUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_VERTEXINFO;
            resource.descriptorSet->UpdateDescriptor(vertexInfoUpdateInfo);

		}

	}
	void RenderResourceManager::ReleaseBindlessID(uint32_t id, BindlessSlot slot)
	{
		m_BindlessIDAlloctor[slot].Release(id);
	}

	RHIDescriptorSetRef RenderResourceManager::GetGlobalResourcePerFrameDescriptorSet()
	{
		return m_PerFrameGlobalResources[APP_FRAMEINDEX].descriptorSet;
	}

	// 更新资源
	void RenderResourceManager::Tick()
	{
		m_SceneInfoFromScene = APP_SCENEMANAGER->GetSceneInfo();
		UpdateCameraInfo();
	}

	void RenderResourceManager::UpdateCameraInfo()
	{
		V2::CameraData tmpdata;
		EditorCamera& camera = m_SceneInfoFromScene.camera;
		tmpdata.view = camera.GetViewMatrix();
		tmpdata.proj = camera.GetProjectionMatrix();
		tmpdata.proj[1][1] *= -1;  // TODO：Y轴反转
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
		// 给这个资源分配一个ID
		uint32_t index = m_BindlessIDAlloctor[slot].Allocate();
		// 更新描述符（每个飞行帧）
		for (auto& resource : m_PerFrameGlobalResources)
		{
			RHIDescriptorUpdateInfo updateInfo = {};
			updateInfo.binding = BindlessSlotToPerFrameBinding(slot),
			updateInfo.index = index;  // bindless数组的index
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


	void RenderResourceManager::InitMultiFrameGlobalResources()
	{
		m_MultiFrameGlobalResources.samplers.push_back(std::make_shared<Sampler>(
			ADDRESS_MODE_CLAMP_TO_EDGE,
			FILTER_TYPE_LINEAR,
			MIPMAP_MODE_LINEAR,
			0.0f));

		m_MultiFrameGlobalResources.samplers.push_back(std::make_shared<Sampler>(
			ADDRESS_MODE_REPEAT,
			FILTER_TYPE_LINEAR,
			MIPMAP_MODE_LINEAR,
			0.0f));

		m_MultiFrameGlobalResources.samplers.push_back(std::make_shared<Sampler>(
			ADDRESS_MODE_CLAMP_TO_EDGE,
			FILTER_TYPE_LINEAR,
			MIPMAP_MODE_NEAREST,
			0.0f,
			SAMPLER_REDUCTION_MODE_MIN));

		m_MultiFrameGlobalResources.samplers.push_back(std::make_shared<Sampler>(
			ADDRESS_MODE_CLAMP_TO_EDGE,
			FILTER_TYPE_LINEAR,
			MIPMAP_MODE_NEAREST,
			0.0f,
			SAMPLER_REDUCTION_MODE_MAX));
		// 为sampler单独创建一个描述符，方便pass使用
		{
			RHIRootSignatureInfo info = {};
			info.AddEntry({ 1, 0, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_SAMPLER });
			m_MultiFrameGlobalResources.samplerRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);

			m_MultiFrameGlobalResources.samplerDescriptorSet = m_MultiFrameGlobalResources.samplerRootSignature->CreateDescriptorSet(1);
			for (uint32_t i = 0; i < m_MultiFrameGlobalResources.samplers.size(); i++)
			{
				RHIDescriptorUpdateInfo updateInfo = {};
				updateInfo.binding = 0;
                updateInfo.index = i;
                updateInfo.resourceType = RESOURCE_TYPE_SAMPLER;
                updateInfo.sampler = m_MultiFrameGlobalResources.samplers[i]->sampler;
				m_MultiFrameGlobalResources.samplerDescriptorSet->UpdateDescriptor(updateInfo);
			}
		}
	}

}