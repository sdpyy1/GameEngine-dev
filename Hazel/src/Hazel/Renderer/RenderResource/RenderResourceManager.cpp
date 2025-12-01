#include "hzpch.h"
#include "RenderResourceManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Scene/SceneManager.h"

namespace GameEngine {
	static uint32_t BindlessSlotToPerFrameBinding(BindlessSlot slot) { return slot + (uint32_t)GLORBAL_RESOURCE_BINDING_BINDLESS_POSITION; }

	RenderResourceManager::RenderResourceManager()
	{
		for (auto& alloctor : m_BindlessIDAlloctor) alloctor = IndexAllocator(MAX_BINDLESS_RESOURCE_SIZE);
		InitMultiFrameGlobalResources();
		InitPerFrameGlobalResources();

		LoadGizmoIcon();


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
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_CUBE, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE }); // 就是得设置Texture，设置Cube不对
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_BINDLESS_TEXTURE_3D, MAX_BINDLESS_RESOURCE_SIZE, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_TEXTURE });


		// 下面这些需要手动去绑定buffer和preFrame资源描述符
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_MESHINFO, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_MATERIALINFO, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_VERTEXINFO, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });

		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_SETTING, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_CAMERA, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_LIGHTINFO, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });
		info.AddEntry({ 0, GLORBAL_RESOURCE_BINDING_GIZMO, 1, SHADER_FREQUENCY_ALL, RESOURCE_TYPE_RW_BUFFER });


		m_GlobalResourcePreFrameRootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		for (auto& resource : m_PerFrameGlobalResources) resource.descriptorSet = m_GlobalResourcePreFrameRootSignature->CreateDescriptorSet(0);

		// 挂载默认全局资源
		for (auto& resource : m_PerFrameGlobalResources) {
			{
				// camera
				RHIDescriptorUpdateInfo cameraUpdateInfo = {};
				cameraUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
				cameraUpdateInfo.buffer = resource.cameraDataBuffer.GetRHIBuffer();
				cameraUpdateInfo.index = 0;
				cameraUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_CAMERA;
				resource.descriptorSet->UpdateDescriptor(cameraUpdateInfo);
			}
			
			
			{
				// Setting
                RHIDescriptorUpdateInfo settingUpdateInfo = {};
                settingUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
                settingUpdateInfo.buffer = m_MultiFrameGlobalResources.globalSettingInfoBuffer.GetRHIBuffer();
                settingUpdateInfo.index = 0;
                settingUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_SETTING;
                resource.descriptorSet->UpdateDescriptor(settingUpdateInfo);
			}

			{
				// meshInfo
				RHIDescriptorUpdateInfo meshInfoUpdateInfo = {};
				meshInfoUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
				meshInfoUpdateInfo.buffer = m_MultiFrameGlobalResources.meshInfoBuffer.GetRHIBuffer();
				meshInfoUpdateInfo.index = 0;
				meshInfoUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_MESHINFO;
				resource.descriptorSet->UpdateDescriptor(meshInfoUpdateInfo);
			}

			{
				// materialInfo
				RHIDescriptorUpdateInfo materialInfoUpdateInfo = {};
				materialInfoUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
				materialInfoUpdateInfo.buffer = m_MultiFrameGlobalResources.materialBuffer.GetRHIBuffer();
				materialInfoUpdateInfo.index = 0;
				materialInfoUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_MATERIALINFO;
				resource.descriptorSet->UpdateDescriptor(materialInfoUpdateInfo);
			}

			{
				// vertexInfo
				RHIDescriptorUpdateInfo vertexInfoUpdateInfo = {};
				vertexInfoUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
				vertexInfoUpdateInfo.buffer = m_MultiFrameGlobalResources.vertexBuffer.GetRHIBuffer();
				vertexInfoUpdateInfo.index = 0;
				vertexInfoUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_VERTEXINFO;
				resource.descriptorSet->UpdateDescriptor(vertexInfoUpdateInfo);
			}

			{
				// sampler
                RHIDescriptorUpdateInfo samplerUpdateInfo = {};
                samplerUpdateInfo.resourceType = RESOURCE_TYPE_SAMPLER;
                samplerUpdateInfo.sampler = m_MultiFrameGlobalResources.samplers[0]->sampler;
                samplerUpdateInfo.index = 0;
                samplerUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER;
                resource.descriptorSet->UpdateDescriptor(samplerUpdateInfo);
				samplerUpdateInfo.sampler = m_MultiFrameGlobalResources.samplers[1]->sampler;
				samplerUpdateInfo.index = 1;
				samplerUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER;
				resource.descriptorSet->UpdateDescriptor(samplerUpdateInfo);
				samplerUpdateInfo.sampler = m_MultiFrameGlobalResources.samplers[2]->sampler;
				samplerUpdateInfo.index = 2;
				samplerUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER;
				resource.descriptorSet->UpdateDescriptor(samplerUpdateInfo);
				samplerUpdateInfo.sampler = m_MultiFrameGlobalResources.samplers[3]->sampler;
				samplerUpdateInfo.index = 3;
				samplerUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_BINDLESS_SAMPLER;
				resource.descriptorSet->UpdateDescriptor(samplerUpdateInfo);
			}
			{
				// lightInfo
				RHIDescriptorUpdateInfo lightUpdateInfo = {};
				lightUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
				lightUpdateInfo.buffer = resource.lightInfoBuffer.GetRHIBuffer();
                lightUpdateInfo.index = 0;
                lightUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_LIGHTINFO;
                resource.descriptorSet->UpdateDescriptor(lightUpdateInfo);
			}
			{
				// gizmo
                RHIDescriptorUpdateInfo gizmoUpdateInfo = {};
                gizmoUpdateInfo.resourceType = RESOURCE_TYPE_RW_BUFFER;
                gizmoUpdateInfo.buffer = resource.gizmoBuffer.GetRHIBuffer();
                gizmoUpdateInfo.index = 0;
                gizmoUpdateInfo.binding = GLORBAL_RESOURCE_BINDING_GIZMO;
                resource.descriptorSet->UpdateDescriptor(gizmoUpdateInfo);
			}
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
		auto setting = APP_SCENEMANAGER->GetSceneInfo().globalSettingInfos;
		setting.iconTextures.dirLightID = m_GlobalSettingInfo.iconTextures.dirLightID;
        setting.iconTextures.pointLightID = m_GlobalSettingInfo.iconTextures.pointLightID;
        setting.iconTextures.spotLightID = m_GlobalSettingInfo.iconTextures.spotLightID;
		SetGlobalSettingInfo(setting);

		cpuRenderSetting = APP_SCENEMANAGER->GetSceneInfo().cpuRenderSetting;

		UpdateCameraInfo();
	}

	void RenderResourceManager::UpdateCameraInfo()
	{
		CameraData tmpdata;
		EditorCamera& camera = APP_SCENEMANAGER->GetSceneInfo().camera;
		tmpdata.view = camera.GetViewMatrix();
		tmpdata.proj = camera.GetProjectionMatrix();
		//tmpdata.proj[1][1] *= -1;  // TODO：Y轴反转
		tmpdata.viewproj = camera.GetViewProjection();
        tmpdata.invPV = glm::inverse(tmpdata.viewproj);
		//tmpdata.Width = camera.GetViewportWidth();
		//tmpdata.Height = camera.GetViewportWidth();
		// 目前统一用窗口的宽高
		tmpdata.Width = APP_WINDOWSIZE.first;
		tmpdata.Height = APP_WINDOWSIZE.second;
		tmpdata.Near = camera.GetNearClip();
		tmpdata.Far = camera.GetFarClip();
		tmpdata.Position = camera.GetPosition();
		tmpdata.padding = 1.f;
		m_PerFrameGlobalResources[APP_FRAMEINDEX].cameraDataBuffer.SetData(tmpdata);
	}

	// TODO: 实时加载新模型，在这里更新所有帧会报错
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

	void RenderResourceManager::SetGizmoDataCommand(void* data, int size)
	{
		m_PerFrameGlobalResources[APP_FRAMEINDEX].gizmoBuffer.SetData(
			data,
			size * sizeof(RHIIndexedIndirectCommand),
			0);
	}

	RHIBufferRef RenderResourceManager::GetGizmoDataBuffer()
	{
		return m_PerFrameGlobalResources[APP_FRAMEINDEX].gizmoBuffer.GetRHIBuffer();
	}



	uint32_t RenderResourceManager::LoadIconFromFile(std::string filePath) {
		TextureSpec spec;
		spec.srgb = false;
		spec.bindless = false; // 因为一些初始化流程原因，在执行内部无法直接申请Bindless,所以在下边手动申请
		spec.path = filePath;
		TextureRef icon = std::make_shared<Texture>(spec);
		BindlessResourceInfo bindlessResourceInfo;
		bindlessResourceInfo.textureView = icon->GetRHITextureView();
		bindlessResourceInfo.resourceType = RESOURCE_TYPE_TEXTURE;
		spec.bindlessId = AllocateBindlessID(bindlessResourceInfo, BINDLESS_SLOT_TEXTURE_2D);
		return spec.bindlessId;
	}

	void RenderResourceManager::LoadGizmoIcon()
	{
		uint32_t pointlightIcon = LoadIconFromFile(APP_ICON_PATH + "pointLight.png");
		uint32_t SpotlightIcon = LoadIconFromFile(APP_ICON_PATH + "Spotlight.png");
		uint32_t directionlightIcon = LoadIconFromFile(APP_ICON_PATH + "sun.png");

		m_GlobalSettingInfo.iconTextures.pointLightID = pointlightIcon;
		m_GlobalSettingInfo.iconTextures.spotLightID = SpotlightIcon;
		m_GlobalSettingInfo.iconTextures.dirLightID = directionlightIcon;
		SetGlobalSettingInfo();
	}

}