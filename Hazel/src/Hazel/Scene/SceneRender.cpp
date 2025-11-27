#include "hzpch.h"
#include "SceneRender.h"
#include "Hazel/Asset/AssetManager.h"
#include <Hazel/Asset/Model/Mesh.h>
#include <glm/gtc/matrix_transform.hpp>
namespace GameEngine {
	void SceneRender::Init()
	{
		m_CommandBuffer = RenderCommandBuffer::Create("PassCommandBuffer");
		InitBuffers();

		InitEnvPass();
		InitAtmospherePass();

		preCompute(); 

		InitDirShadowPass();
		InitSpotShadowPass();
		InitPreDepthPass();
		InitHZBPass();
		InitGeoPass();
		InitLightPass();
		InitSkyPass();
		InitBloomPass();
		InitSceneCompositePass();
		InitGridPass();	
	}	
	
	void SceneRender::Draw() {
		m_EnvTextures = m_EnvPass.compute(m_SceneDataFromScene.SceneLightEnvironment.SkyLightSetting.selelctEnvPath, m_CommandBuffer);
		MultiScatteringLutPass();
		SkyViewLutPass();
		ShadowPass();
		SpotShadowPass();
		PreDepthPass();
		// HZBComputePass();
		GeoPass();
		LightPass();
		SkyPass();
		BloomPass();
		SceneCompositePass();
		GridPass();
	}
	void SceneRender::PreRender(SceneInfo sceneData)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		// ���ճ�������
		m_SceneDataFromScene = sceneData;
		// ����Resize
		HandleResizeRuntime();
		// ���¸�����Դ��Ϣ
		UploadCameraData(); // ���������
		UploadMeshAndBoneTransForm(); // ģ�ͱ任�͹����任����
		UploadCSMShadowData(); // ������Ӱ����
		UploadSpotShadowData(); // �۹���Ӱ
		UploadRenderSettingData();  // ��Ⱦ��������
		uploadSceneData(); // ��������
	}
	void SceneRender::EndRender()
	{
		m_CommandBuffer->Begin();
		Draw();
		m_CommandBuffer->End();
		m_CommandBuffer->Submit();

		m_DynamicDrawList.clear();
		m_StaticMeshDrawList.clear();
		m_MeshTransformMap.clear();
		m_MeshBoneTransformsMap.clear();
	}

	void SceneRender::GeoPass()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		// ��̬����
		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoPass, false);
		for (auto& [meshKey, drawCommand] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(meshKey);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeoPipeline, drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, drawCommand.InstanceCount);
		}
		Renderer::EndRenderPass(m_CommandBuffer);

		// ��̬����
		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoAnimPass, false);
		for (auto& [meshKey, drawCommand] : m_DynamicDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(meshKey);
			if (drawCommand.IsRigged) {
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(meshKey);
				Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_GeoAnimPipeline, drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, drawCommand.InstanceCount);
			}
			else {
				Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_GeoAnimPipeline, drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, drawCommand.InstanceCount);
			}
		}
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRender::CalculateCascades(CascadeDataold* cascades, const EditorCamera& sceneCamera, const glm::vec3& lightDirection) const
	{
		float nearOffset = -250.f;
		float farOffset = 0.f;
		glm::mat4 viewProjection = sceneCamera.GetViewProjection();
		float CascadeSplitLambda = 0.9f;
		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
		float nearClip = sceneCamera.GetNearClip();
		float farClip = sceneCamera.GetFarClip();
		float clipRange = farClip - nearClip;
		float minZ = nearClip;
		float maxZ = nearClip + clipRange;
		float range = maxZ - minZ;
		float ratio = maxZ / minZ;
		
		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}
	
		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}
			// frustumCorners���ó�һ��������8������λ��
			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// ���㼶�����������꣨8�������ƽ��ֵ��
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			// ���㼶���İ뾶�����ĵ㵽�����ǵ��������ֵ��
			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			// ����ȡ��������� 1/16 ������(??)
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDirection * radius, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f)); // ���ն���Ҳ�������view��ת�Ƶ���ռ䣬����Up����Ӱ��
			glm::mat4 lightOrthoMatrix = glm::ortho(-radius, radius, -radius, radius, 0.0f + nearOffset, radius * 2 + farOffset); // TODO:��Χ��ƻ���Ҫ���Ӿ�ϸ������ƣ�offsetĿǰ��ƺܴ���ܰ������һ������

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = (float)m_DirectionalShadowMapPass[0]->GetSpecification().Pipeline->GetSpecification().TargetFramebuffer->GetWidth();

			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;  // TODO:����Ż���û����roundOffset��ص�

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange);
			cascades[i].ViewProj = shadowMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}
	void SceneRender::InitSpotShadowPass()
	{
		FramebufferSpecification framebufferSpec;
		framebufferSpec.Width = shadowMapResolution;
		framebufferSpec.Height = shadowMapResolution;
		framebufferSpec.Attachments = { ImageFormat::DEPTH32F };
		framebufferSpec.DepthClearValue = 1.0f;
		framebufferSpec.NoResize = true;
		framebufferSpec.DebugName = "SpotShadowMap";

		auto shadowPassShader = Renderer::GetShaderLibrary()->Get("SpotShadowMap");
		auto shadowPassShaderAnim = Renderer::GetShaderLibrary()->Get("SpotShadowMapAnim");
		m_SpotFrameBuffer = Framebuffer::Create(framebufferSpec);
		framebufferSpec.DebugName = "SpotShadowMapAnim";
		framebufferSpec.ClearDepthOnLoad = false;
		framebufferSpec.ExistingImages[0] = m_SpotFrameBuffer->GetDepthImage();
        m_SpotFrameAnimBuffer = Framebuffer::Create(framebufferSpec);
		PipelineSpecification pipelineSpec;
		pipelineSpec.DebugName = "SpotShadowPass";
		pipelineSpec.Shader = shadowPassShader;
		pipelineSpec.TargetFramebuffer = m_SpotFrameBuffer;
		pipelineSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
		pipelineSpec.Layout = vertexLayout;
		pipelineSpec.InstanceLayout = instanceLayout;
		PipelineSpecification pipelineSpecAnim = pipelineSpec;
		pipelineSpecAnim.DebugName = "SpotShadowPassAnim";
		pipelineSpecAnim.Shader = shadowPassShaderAnim;
		pipelineSpecAnim.BoneInfluenceLayout = boneInfluenceLayout;
		pipelineSpecAnim.TargetFramebuffer = m_SpotFrameAnimBuffer;
		m_SpotShadowPassPipeline = Pipeline::Create(pipelineSpec);
		m_SpotShadowPassAnimPipeline = Pipeline::Create(pipelineSpecAnim);

		RenderPassSpecification spotShadowPassSpec;
		spotShadowPassSpec.DebugName = "SpotShadowMap";
		spotShadowPassSpec.Pipeline = m_SpotShadowPassPipeline;
		m_SpotShadowPass = RenderPass::Create(spotShadowPassSpec);
		spotShadowPassSpec.DebugName = "SpotShadowMapAnim";
		spotShadowPassSpec.Pipeline = m_SpotShadowPassAnimPipeline;
		m_SpotShadowAnimPass = RenderPass::Create(spotShadowPassSpec);
		m_SpotShadowPass->SetInput("u_SpotLightMatrices",m_UBSSpotLightMatrixData);
		m_SpotShadowAnimPass->SetInput("u_SpotLightMatrices", m_UBSSpotLightMatrixData);
		m_SpotShadowAnimPass->SetInput("r_BoneTransforms",m_SBSBoneTransforms);
	}
	void SceneRender::UploadSpotShadowData()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const std::vector<SpotLight>& spotLightsVec = m_SceneDataFromScene.SceneLightEnvironment.SpotLights;
		for (uint32_t i = 0; i < spotLightsVec.size(); i++) {
			glm::mat4 viewMatrix = glm::lookAt(spotLightsVec[i].Position, spotLightsVec[i].Position + spotLightsVec[i].Direction, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 projection = glm::perspective(glm::radians(spotLightsVec[i].Angle), 1.f, 0.1f, spotLightsVec[i].Range);
			m_SpotLightMatrixData[frameIndex].ShadowMatrices[i] = projection * viewMatrix;
		}
        m_UBSSpotLightMatrixData->Get()->SetData(&m_SpotLightMatrixData[frameIndex], sizeof(SpotLightMatrixs));
	}
	void SceneRender::SpotShadowPass()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		const std::vector<SpotLight>& spotLights = m_SceneDataFromScene.SceneLightEnvironment.SpotLights;
		if (spotLights.size() == 0) return;
		for (uint32_t i = 0; i < spotLights.size(); i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_SpotShadowPass);

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_StaticMeshDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelines[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, cascade);
			}
			Renderer::EndRenderPass(m_CommandBuffer);
		}
		for (uint32_t i = 0; i < spotLights.size(); i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_SpotShadowAnimPass);

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_DynamicDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelinesAnim[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount, cascade);
				}
				else {
					Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelinesAnim[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount, cascade);
				}
			}
			Renderer::EndRenderPass(m_CommandBuffer);
		}
	}
	void SceneRender::UploadCSMShadowData() {
        uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		if (m_SceneDataFromScene.SceneLightEnvironment.DirectionalLights[0].Intensity == 0) return;
		CascadeDataold cascades[4];
		CalculateCascades(cascades, m_SceneDataFromScene.camera, m_SceneDataFromScene.SceneLightEnvironment.DirectionalLights[0].Direction);
		for (int i = 0; i < NumShadowCascades; i++)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			m_ViewProjToLigthData[frameIndex].ViewProjection[i] = cascades[i].ViewProj;
		}
		m_UBSViewProjToLight->Get()->SetData(&m_ViewProjToLigthData[frameIndex], sizeof(UBViewProjToLight));
	}
	void SceneRender::ShadowPass() {
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		auto& directionalLights = m_SceneDataFromScene.SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Intensity == 0.0f)
		{
			return;
		}
		for (uint32_t i = 0; i < NumShadowCascades; i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_DirectionalShadowMapPass[i]);

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_StaticMeshDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelines[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, cascade);
			}
			Renderer::EndRenderPass(m_CommandBuffer);
		}
		for (uint32_t i = 0; i < NumShadowCascades; i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_DirectionalShadowMapAnimPass[i]);

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_DynamicDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelinesAnim[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount, cascade);
				}
				else {
					Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelinesAnim[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount, cascade);
				}
			}
			Renderer::EndRenderPass(m_CommandBuffer);
		}
	}
	void SceneRender::InitPreDepthPass() {
		FramebufferSpecification preDepthFramebufferSpec;
		preDepthFramebufferSpec.DebugName = "PreDepth";
		preDepthFramebufferSpec.Attachments = { ImageFormat::Depth };
		preDepthFramebufferSpec.DepthClearValue = 1.0f;
		m_PreDepthClearFramebuffer = Framebuffer::Create(preDepthFramebufferSpec);
		preDepthFramebufferSpec.ClearDepthOnLoad = false;
		preDepthFramebufferSpec.ExistingImages[0] = m_PreDepthClearFramebuffer->GetDepthImage();
		preDepthFramebufferSpec.DebugName = "PreDepthAnim";
		m_PreDepthLoadFramebuffer = Framebuffer::Create(preDepthFramebufferSpec);
		PipelineSpecification pipelineSpec;
		pipelineSpec.DebugName = "PreDepth";
		pipelineSpec.TargetFramebuffer = m_PreDepthClearFramebuffer;
		pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepth");
		pipelineSpec.Layout = vertexLayout;
		pipelineSpec.InstanceLayout = instanceLayout;
		m_PreDepthPipeline = Pipeline::Create(pipelineSpec);
		pipelineSpec.TargetFramebuffer = m_PreDepthLoadFramebuffer;
		pipelineSpec.DebugName = "PreDepth-Anim";
		pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepthAnim");
		pipelineSpec.BoneInfluenceLayout = boneInfluenceLayout;
		m_PreDepthPipelineAnim = Pipeline::Create(pipelineSpec);
		RenderPassSpecification preDepthRenderPassSpec;
		preDepthRenderPassSpec.DebugName = "PreDepth";
		preDepthRenderPassSpec.Pipeline = m_PreDepthPipeline;
		m_PreDepthPass = RenderPass::Create(preDepthRenderPassSpec);
		preDepthRenderPassSpec.DebugName = "PreDepth-Anim";
		preDepthRenderPassSpec.Pipeline = m_PreDepthPipelineAnim;
		m_PreDepthAnimPass = RenderPass::Create(preDepthRenderPassSpec);

		m_PreDepthPass->SetInput("u_CameraData",m_UBSCameraData);
		m_PreDepthAnimPass->SetInput("u_CameraData", m_UBSCameraData);
		m_PreDepthAnimPass->SetInput("r_BoneTransforms",m_SBSBoneTransforms);
	}
	void SceneRender::PreDepthPass() {
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthPass);
		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_PreDepthPipeline, dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount);
		}
		Renderer::EndRenderPass(m_CommandBuffer);
		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthAnimPass);
		for (auto& [mk, dc] : m_DynamicDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_PreDepthPipelineAnim, dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount);
			}
			else {
				Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_PreDepthPipelineAnim, dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount);
			}
		}
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::GridPass()
	{
		m_GridPass->SetInput("inDepth", m_GeoAnimPass->GetDepthOutput());
		Renderer::BeginRenderPass(m_CommandBuffer, m_GridPass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer, 6);
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::SceneCompositePass()
	{
		m_SceneCompositePass->SetInput("lightRes",m_LightPassFramebuffer->GetImage(0));
		m_SceneCompositePass->SetInput("BloomRes",m_BloomImage->GetImage());
		Renderer::BeginRenderPass(m_CommandBuffer, m_SceneCompositePass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer, 3);
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::UploadCameraData()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_CameraData[frameIndex].view = m_SceneDataFromScene.camera.GetViewMatrix();
		m_CameraData[frameIndex].proj = m_SceneDataFromScene.camera.GetProjectionMatrix();
		m_CameraData[frameIndex].proj[1][1] *= -1; // Y�ᷴת
		m_CameraData[frameIndex].viewproj = m_CameraData[frameIndex].proj * m_CameraData[frameIndex].view;
		m_CameraData[frameIndex].Near = m_SceneDataFromScene.camera.GetNearClip();
		m_CameraData[frameIndex].Far = m_SceneDataFromScene.camera.GetFarClip();
		m_CameraData[frameIndex].Width = m_SceneDataFromScene.camera.GetViewportWidth();
		m_CameraData[frameIndex].Height = m_SceneDataFromScene.camera.GetViewportHeight();
		m_CameraData[frameIndex].Position = m_SceneDataFromScene.camera.GetPosition();
		m_CameraData[frameIndex].InverseViewProj = glm::inverse(m_CameraData[frameIndex].viewproj);
		m_UBSCameraData->Get()->SetData(&m_CameraData[frameIndex], sizeof(CameraData));
	}
	void SceneRender::SubmitStaticMesh(Ref<MeshSource> meshSource, const glm::mat4& transform) {
		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex = 0; submeshIndex < submeshData.size(); submeshIndex++) {
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;  // subMesh��ȫ�ֱ任
			AssetHandle materialHandle = meshSource->GetMaterialHandle(submeshData[submeshIndex].MaterialIndex);
			Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);// subMesh�Ĳ�������;
			VERIFY(material);
			MeshKey meshKey = { meshSource->Handle, materialHandle, submeshIndex, false };
			// ����任����
			auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back(); // ����ÿһ��MeshKey�����洢���Transforms���󣬱�ʾ���ʵ��
			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };
			// �����������
			StaticDrawCommand& drawCommand = m_StaticMeshDrawList[meshKey];
			drawCommand.MeshSource = meshSource;
			drawCommand.SubmeshIndex = submeshIndex;
			drawCommand.MaterialAsset = material;
			drawCommand.InstanceCount++;
		}
	};
	void SceneRender::SubmitDynamicMesh(Ref<MeshSource> meshSource, uint32_t submeshIndex, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms)
	{
		// TODO: Culling, sorting, etc.

		const auto& submeshes = meshSource->GetSubmeshes();
		const auto& submesh = submeshes[submeshIndex];
		uint32_t materialIndex = submesh.MaterialIndex;
		bool isRigged = submesh.IsRigged;

		AssetHandle materialHandle = meshSource->GetMaterialHandle(submeshes[submeshIndex].MaterialIndex);
		Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

		MeshKey meshKey = { meshSource->Handle, materialHandle, submeshIndex, false };
		auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		if (isRigged)
		{
			CopyToBoneTransformStorage(meshKey, meshSource, boneTransforms);
		}
		// Main geo
		{
			DynamicDrawCommand& drawCommand = m_DynamicDrawList[meshKey];
			drawCommand.MeshSource = meshSource;
			drawCommand.SubmeshIndex = submeshIndex;
			drawCommand.InstanceCount++;
			drawCommand.MaterialAsset = material;
			drawCommand.IsRigged = isRigged;  // TODO: would it be better to have separate draw list for rigged meshes, or this flag is OK?
		}
	}
	void SceneRender::CopyToBoneTransformStorage(const MeshKey& meshKey, const Ref<MeshSource>& meshSource, const std::vector<glm::mat4>& boneTransforms)
	{
		auto& boneTransformStorage = m_MeshBoneTransformsMap[meshKey].BoneTransformsData.emplace_back();
		if (boneTransforms.empty())
		{
			boneTransformStorage.fill(glm::identity<glm::mat4>());
		}
		else
		{
			for (size_t i = 0; i < meshSource->m_BoneInfo.size(); ++i)
			{
				// 1. ��任����ת�Ƶ������ռ� 2. �������������ռ��ƶ� 3. ȫ�ֱ任
				boneTransformStorage[i] = meshSource->GetSkeleton()->GetTransform() * boneTransforms[meshSource->m_BoneInfo[i].BoneIndex] * meshSource->m_BoneInfo[i].InverseBindPose;
			}
		}
	}
	void SceneRender::SetViewprotSize(float width, float height) {
		if (width != m_ViewportWidth || height != m_ViewportHeight)
		{
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			NeedResize = true;
		}
	}
	void SceneRender::InitBuffers()
	{
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		// MVP�����UBO
		m_CameraData.resize(framesInFlight);
		m_UBSCameraData = UniformBufferSet::Create(sizeof(CameraData), "CameraData");

		m_ViewProjToLigthData.resize(framesInFlight);
		m_UBSViewProjToLight = UniformBufferSet::Create(sizeof(UBViewProjToLight), "Shadow");

		m_RenderSettingData.resize(framesInFlight);
        m_UBSRenderSetting = UniformBufferSet::Create(sizeof(RenderSettingData), "RenderSettingData");

		m_SceneDataForShader.resize(framesInFlight);
        m_UBSSceneDataForShader = UniformBufferSet::Create(sizeof(SceneDataForShader), "SceneDataForShader");
		
		m_SpotLightMatrixData.resize(framesInFlight);
        m_UBSSpotLightMatrixData = UniformBufferSet::Create(sizeof(SpotLightMatrixs), "SpotLightMatrixData");


		m_SubmeshTransformBuffers.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			m_SubmeshTransformBuffers[i].Buffer = VertexBuffer::Create(sizeof(TransformVertexData) * TransformBufferCount, "TransformBuffers");
			m_SubmeshTransformBuffers[i].Data = new TransformVertexData[TransformBufferCount];
		}
		{
			StorageBufferSpecification spec;
			spec.DebugName = "BoneTransforms";
			spec.GPUOnly = false;
			const size_t BoneTransformBufferCount = 1 * 1024; // basically means limited to 1024 animated meshes   TODO(0x): resizeable/flushable
			m_SBSBoneTransforms = StorageBufferSet::Create(spec, sizeof(BoneTransforms) * BoneTransformBufferCount);
			m_BoneTransformsData = new BoneTransforms[BoneTransformBufferCount];
		}
	}
	void SceneRender::InitDirShadowPass() {
		// һ�Ŷ�Layer���ͼ���ڲ�ͬ����Ӱ��
		ImageSpecification spec;
		spec.Format = ImageFormat::DEPTH32F;
		spec.Usage = ImageUsage::Attachment;
		spec.Width = shadowMapResolution;
		spec.Height = shadowMapResolution;
		spec.Layers = NumShadowCascades;
		spec.DebugName = "ShadowCascades";
		Ref<Image2D> cascadedDepthImage = Image2D::Create(spec);
		cascadedDepthImage->Invalidate();
		if (NumShadowCascades > 1)
			cascadedDepthImage->CreatePerLayerImageViews();
		FramebufferSpecification shadowMapFramebufferSpec;
		shadowMapFramebufferSpec.DebugName = "DirShadowMap";
		shadowMapFramebufferSpec.Width = shadowMapResolution;
		shadowMapFramebufferSpec.Height = shadowMapResolution;
		shadowMapFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
		shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		shadowMapFramebufferSpec.DepthClearValue = 1.0f;
		shadowMapFramebufferSpec.NoResize = true;
		shadowMapFramebufferSpec.ExistingImage = cascadedDepthImage;

		PipelineSpecification pipelineSpec;
		pipelineSpec.DebugName = "DirShadowPass";
		pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("DirShadowMap");
		pipelineSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
		pipelineSpec.Layout = vertexLayout;
		pipelineSpec.InstanceLayout = instanceLayout;

		PipelineSpecification pipelineSpecAnim = pipelineSpec;
		pipelineSpecAnim.DebugName = "DirShadowPass-Anim";
		pipelineSpecAnim.Shader = Renderer::GetShaderLibrary()->Get("DirShadowMapAnim");
		pipelineSpecAnim.BoneInfluenceLayout = boneInfluenceLayout;
		RenderPassSpecification shadowMapRenderPassSpec;
		m_DirectionalShadowMapPass.resize(NumShadowCascades);
		m_DirectionalShadowMapAnimPass.resize(NumShadowCascades);
		for (uint32_t i = 0; i < NumShadowCascades; i++)
		{
			shadowMapRenderPassSpec.DebugName = shadowMapFramebufferSpec.DebugName;

			shadowMapFramebufferSpec.ExistingImageLayers.clear();
			shadowMapFramebufferSpec.ExistingImageLayers.emplace_back(i);

			shadowMapFramebufferSpec.ClearDepthOnLoad = true;
			pipelineSpec.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);

			m_ShadowPassPipelines[i] = Pipeline::Create(pipelineSpec);
			shadowMapRenderPassSpec.DebugName = "DirShadowPass";
			shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelines[i];

			shadowMapFramebufferSpec.ClearDepthOnLoad = false;
			pipelineSpecAnim.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);
			m_ShadowPassPipelinesAnim[i] = Pipeline::Create(pipelineSpecAnim);

			m_DirectionalShadowMapPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
			m_DirectionalShadowMapPass[i]->SetInput("u_DirShadow",m_UBSViewProjToLight);

			shadowMapRenderPassSpec.DebugName = "DirShadowPassAnim";
			shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelinesAnim[i];
			m_DirectionalShadowMapAnimPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
			m_DirectionalShadowMapAnimPass[i]->SetInput("u_DirShadow", m_UBSViewProjToLight);
			m_DirectionalShadowMapAnimPass[i]->SetInput("r_BoneTransforms",m_SBSBoneTransforms);
		}
	}
	void SceneRender::InitGeoPass() {
		{
			FramebufferSpecification framebufferSpec;
			// pos normal albedo mr depth
			framebufferSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::Depth };
			framebufferSpec.DebugName = "GBuffer";
			framebufferSpec.ExistingImages[4] = m_PreDepthLoadFramebuffer->GetDepthImage();
			framebufferSpec.ClearDepthOnLoad = false;
			m_GeoFrameBuffer = Framebuffer::Create(framebufferSpec);
			PipelineSpecification pSpec;
			pSpec.Layout = vertexLayout;
			pSpec.InstanceLayout = instanceLayout;
			pSpec.Shader = Renderer::GetShaderLibrary()->Get("gBuffer");
			pSpec.TargetFramebuffer = m_GeoFrameBuffer;
			pSpec.DebugName = "GbufferPipeline";
			pSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
			m_GeoPipeline = Pipeline::Create(pSpec);
			RenderPassSpecification gBufferPassSpec;
			gBufferPassSpec.Pipeline = m_GeoPipeline;
			gBufferPassSpec.DebugName = "gBufferPass";
			m_GeoPass = RenderPass::Create(gBufferPassSpec);
			m_GeoPass->SetInput("u_CameraData", m_UBSCameraData);
			m_GeoPass->SetInput("u_RendererData",m_UBSRenderSetting);
			m_GeoPass->SetInput("u_Scene",m_UBSSceneDataForShader);
			m_GeoPass->SetInput("u_DirShadow",m_UBSViewProjToLight);
		}
		// GeoAnimPass
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::Depth };
			framebufferSpec.DebugName = "GBufferAmin";
			framebufferSpec.ClearDepthOnLoad = false;
			framebufferSpec.ClearColorOnLoad = false;
			framebufferSpec.ExistingImages[0] = m_GeoFrameBuffer->GetImage(0);
			framebufferSpec.ExistingImages[1] = m_GeoFrameBuffer->GetImage(1);
			framebufferSpec.ExistingImages[2] = m_GeoFrameBuffer->GetImage(2);
			framebufferSpec.ExistingImages[3] = m_GeoFrameBuffer->GetImage(3);
			framebufferSpec.ExistingImages[4] = m_GeoFrameBuffer->GetDepthImage();
			m_GeoAnimFrameBuffer = Framebuffer::Create(framebufferSpec);
			PipelineSpecification pSpec;
			pSpec.Layout = vertexLayout;
			pSpec.InstanceLayout = instanceLayout;
			pSpec.BoneInfluenceLayout = boneInfluenceLayout;
			pSpec.Shader = Renderer::GetShaderLibrary()->Get("gBufferAnim");
			pSpec.TargetFramebuffer = m_GeoAnimFrameBuffer;
			pSpec.DebugName = "GbufferAnimPipeline";
			pSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
			m_GeoAnimPipeline = Pipeline::Create(pSpec);
			RenderPassSpecification gBufferPassSpec;
			gBufferPassSpec.Pipeline = m_GeoAnimPipeline;
			gBufferPassSpec.DebugName = "gBufferAnimPass";
			m_GeoAnimPass = RenderPass::Create(gBufferPassSpec);
			m_GeoAnimPass->SetInput("u_CameraData", m_UBSCameraData);
			m_GeoAnimPass->SetInput("r_BoneTransforms",m_SBSBoneTransforms);
			m_GeoAnimPass->SetInput("u_RendererData", m_UBSRenderSetting);
			m_GeoAnimPass->SetInput("u_Scene", m_UBSSceneDataForShader);
			m_GeoAnimPass->SetInput("u_DirShadow", m_UBSViewProjToLight);
		}
	}
	void SceneRender::InitGridPass() {
		FramebufferTextureSpecification gridColorOutputSpec(ImageFormat::RGBA32F);
		FramebufferSpecification gridPassFramebufferSpec;
		gridPassFramebufferSpec.Attachments = { gridColorOutputSpec };
		gridPassFramebufferSpec.DebugName = "Grid";
		gridPassFramebufferSpec.ExistingImages[0] = m_SceneCompositeFrameBuffer->GetImage(0); 
		gridPassFramebufferSpec.Attachments.Attachments[0].LoadOp = AttachmentLoadOp::Load;
		m_GridFrameBuffer = Framebuffer::Create(gridPassFramebufferSpec);
		PipelineSpecification gridPipelineSpec;
		gridPipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("grid");
		gridPipelineSpec.TargetFramebuffer = m_GridFrameBuffer;
		gridPipelineSpec.DepthTest = false;
		gridPipelineSpec.DebugName = "GridPipeline";
		m_GridPipeline = Pipeline::Create(gridPipelineSpec);
		RenderPassSpecification gridPassSpec;
		gridPassSpec.Pipeline = m_GridPipeline;
		gridPassSpec.DebugName = "gridPass";
		m_GridPass = RenderPass::Create(gridPassSpec);
		m_GridPass->SetInput("u_CameraData", m_UBSCameraData); // ����binding=0��ubo
		m_GridPass->SetInput("inDepth",m_GeoAnimPass->GetDepthOutput());
	}
	void SceneRender::UploadMeshAndBoneTransForm() {
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

			uint32_t offset = 0;
			for (auto& [key, transformData] : m_MeshTransformMap)
			{
				transformData.TransformOffset = offset * sizeof(TransformVertexData);
				for (const auto& transform : transformData.Transforms)
				{
					m_SubmeshTransformBuffers[frameIndex].Data[offset] = transform;
					offset++;
				}
			}
			m_SubmeshTransformBuffers[frameIndex].Buffer->SetData(m_SubmeshTransformBuffers[frameIndex].Data, offset * sizeof(TransformVertexData));
		}

		uint32_t index = 0;
		for (auto& [key, boneTransformsData] : m_MeshBoneTransformsMap)
		{
			boneTransformsData.BoneTransformsBaseIndex = index;
			for (const auto& boneTransforms : boneTransformsData.BoneTransformsData)
			{
				m_BoneTransformsData[index++] = boneTransforms;
			}
		}

		if (index > 0)
		{
			SceneRender* instance = this;
			RENDER_SUBMIT([instance, index]() mutable
				{
					instance->m_SBSBoneTransforms->RT_Get()->RT_SetData(instance->m_BoneTransformsData, static_cast<uint32_t>(index * sizeof(BoneTransforms)));
				});
		}
	}
	void SceneRender::HandleResizeRuntime() {
		SetViewprotSize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
		if (NeedResize) {
			LOG_WARN("SceneRender::PreRender Resize FBO to {0}x{1}", m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			m_PreDepthClearFramebuffer->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			m_PreDepthLoadFramebuffer->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			m_GeoFrameBuffer->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			m_GeoAnimFrameBuffer->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			m_LightPassFramebuffer->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight()); 
			m_SkyFrameBuffer->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			m_BloomImage->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			m_SceneCompositeFrameBuffer->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			m_GridFrameBuffer->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
			HandleHZBResize();

			// Bloom Resize
			m_BloomImageViews.clear();
			m_BloomImageViews.resize(m_BloomImage->GetMipLevelCount());
			ImageViewSpecification imageViewSpec;
			m_BloomPreDownSamplerMaterials.clear();
			m_BloomPreUpSamplerMaterials.clear();
			m_BloomPreDownSamplerMaterials.resize(m_BloomImage->GetMipLevelCount());
			m_BloomPreUpSamplerMaterials.resize(m_BloomImage->GetMipLevelCount());
			for (int i = 0; i < m_BloomImage->GetMipLevelCount(); i++) {
				imageViewSpec.DebugName = "BloomImageView" + std::to_string(i);
				imageViewSpec.Image = m_BloomImage->GetImage();
				imageViewSpec.Mip = i;
				m_BloomImageViews[i] = ImageView::Create(imageViewSpec);
				m_BloomPreDownSamplerMaterials[i] = MaterialOld::Create(m_BloomShader);
				m_BloomPreUpSamplerMaterials[i] = MaterialOld::Create(m_BloomShader);
			}



			NeedResize = false;
		}
	}

	void SceneRender::ClearPass(Ref<RenderPass> renderPass, bool explicitClear)
	{
		Renderer::BeginRenderPass(m_CommandBuffer, renderPass, explicitClear);
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::HandleHZBResize()
	{
		m_HierarchicalDepthTexture.Texture->Resize(m_SceneDataFromScene.camera.GetViewportWidth(), m_SceneDataFromScene.camera.GetViewportHeight());
		uint32_t mipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
		m_HierarchicalDepthTexture.ImageViews.resize(m_HierarchicalDepthTexture.Texture->GetMipLevelCount());
		ImageViewSpecification imageViewSpec;
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			imageViewSpec.DebugName = fmt::format("HierarchicalDepthTexture-{}", mip);
			imageViewSpec.Image = m_HierarchicalDepthTexture.Texture->GetImage();
			imageViewSpec.Mip = mip;
			m_HierarchicalDepthTexture.ImageViews[mip] = ImageView::Create(imageViewSpec);
		}
	}
	void SceneRender::InitHZBPass()
	{
		TextureSpecification spec;
		spec.Format = ImageFormat::RED32F;
		spec.Width = 1;
		spec.Height = 1;
		spec.SamplerWrap = TextureWrap::Clamp;
		spec.SamplerFilter = TextureFilter::Nearest;
		spec.DebugName = "HierarchicalZ";
		spec.Storage = true;
		m_HierarchicalDepthTexture.Texture = Texture2D::Create(spec);
		uint32_t mipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
		m_HierarchicalDepthTexture.ImageViews.resize(m_HierarchicalDepthTexture.Texture->GetMipLevelCount());
		ImageViewSpecification imageViewSpec;
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			imageViewSpec.DebugName = fmt::format("HierarchicalDepthTexture-{}", mip);
			imageViewSpec.Image = m_HierarchicalDepthTexture.Texture->GetImage();
			imageViewSpec.Mip = mip;
			m_HierarchicalDepthTexture.ImageViews[mip] = ImageView::Create(imageViewSpec);
		}
		Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("HZB");
		ComputePassSpecification hdPassSpec;
		hdPassSpec.DebugName = "HierarchicalDepth";
		hdPassSpec.Pipeline = PipelineCompute::Create(shader);
		m_HierarchicalDepthPass = ComputePass::Create(hdPassSpec);
	}
	void SceneRender::HZBComputePass()
	{
		uint32_t inputWidth = m_PreDepthLoadFramebuffer->GetDepthImage()->GetWidth();
		uint32_t inputHeight = m_PreDepthLoadFramebuffer->GetDepthImage()->GetHeight();

		// ����������������0���ߴ磩
		const uint32_t localSize = 8;
		uint32_t groupCountX = (inputWidth + localSize - 1) / localSize;
		uint32_t groupCountY = (inputHeight + localSize - 1) / localSize;

		// ����Դ���������ͼ����������+ HZB���飨�洢ͼ��
		Renderer::BeginComputePass(m_CommandBuffer, m_HierarchicalDepthPass);
		m_HierarchicalDepthPass->SetInput("u_InputDepth",m_PreDepthLoadFramebuffer->GetDepthImage());
		uint32_t mipLevels = 0;
		for (mipLevels = 0; mipLevels < m_HierarchicalDepthTexture.ImageViews.size(); mipLevels++) {
			m_HierarchicalDepthPass->SetInputOneLayer(m_HierarchicalDepthTexture.ImageViews[mipLevels], 1, mipLevels);
		}

		const Buffer mip(&mipLevels, sizeof(uint32_t));

		Renderer::DispatchCompute(m_CommandBuffer, m_HierarchicalDepthPass, nullptr, glm::uvec3(groupCountX, groupCountY, 1), mip);
		Renderer::EndComputePass(m_CommandBuffer, m_HierarchicalDepthPass);
	}
	void SceneRender::InitEnvPass()
	{
		m_EnvPass.Init();
	}

	SceneRender::SceneRender()
	{
		Init();
	}


	void SceneRender::InitLightPass()
	{
		FramebufferSpecification lightPassFramebufferSpec;
		lightPassFramebufferSpec.Attachments = { ImageFormat::RGBA32F };
		lightPassFramebufferSpec.DebugName = "LightPass";
		m_LightPassFramebuffer = Framebuffer::Create(lightPassFramebufferSpec);
		PipelineSpecification pSpec;
		pSpec.Shader = Renderer::GetShaderLibrary()->Get("Lighting");
		pSpec.TargetFramebuffer = m_LightPassFramebuffer;
		pSpec.DebugName = "LightPassPipeline";
		pSpec.DepthTest = false;
		m_LightPassPipeline = Pipeline::Create(pSpec);
		RenderPassSpecification lightPassSpec;
		lightPassSpec.Pipeline = m_LightPassPipeline;
		lightPassSpec.DebugName = "LightPass";
		m_LightPass = RenderPass::Create(lightPassSpec);
        m_LightPass->SetInput("u_CameraData",m_UBSCameraData);

        m_LightPass->SetInput("u_RendererData",m_UBSRenderSetting);
		m_LightPass->SetInput("u_Scene",m_UBSSceneDataForShader);
        m_LightPass->SetInput("u_DirShadow",m_UBSViewProjToLight);
		m_LightPass->SetInput("u_DirShadowMapTexture",m_ShadowPassPipelines[0]->GetSpecification().TargetFramebuffer->GetDepthImage(),true);


	}
	void SceneRender::LightPass()
	{
		m_LightPass->SetInput("u_EnvRadianceTex", m_EnvTextures.m_EnvPreFilterMap);
		m_LightPass->SetInput("u_EnvIrradianceTex", m_EnvTextures.m_EnvIrradianceMap);
		m_LightPass->SetInput("u_BRDFLUTTexture", m_EnvTextures.m_EnvLut);
		m_LightPass->SetInput("u_AlbedoTexture",m_GeoFrameBuffer->GetImage(0));
		m_LightPass->SetInput("u_PositionTexture",m_GeoFrameBuffer->GetImage(1));
		m_LightPass->SetInput("u_NormalTexture",m_GeoFrameBuffer->GetImage(2));
		m_LightPass->SetInput("u_MRTexture",m_GeoFrameBuffer->GetImage(3));

		Renderer::BeginRenderPass(m_CommandBuffer, m_LightPass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer, 3);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRender::UploadRenderSettingData()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_RenderSettingData[frameIndex].CascadeSplits = CascadeSplits;
		m_RenderSettingData[frameIndex].ShadowType = m_SceneDataFromScene.RenderSettingData.ShadowType;
		m_RenderSettingData[frameIndex].deBugCSM = m_SceneDataFromScene.RenderSettingData.deBugCSM;
		m_RenderSettingData[frameIndex].bloomScale = m_SceneDataFromScene.RenderSettingData.bloomScale;
		m_UBSRenderSetting->Get()->SetData(&m_RenderSettingData[frameIndex], sizeof(RenderSettingData));
	}

	void SceneRender::uploadSceneData()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_SceneDataForShader[frameIndex].DirectionalLight = m_SceneDataFromScene.SceneLightEnvironment.DirectionalLights[0];
		m_SceneDataForShader[frameIndex].EnvironmentMapIntensity = 1.f;
		m_SceneDataForShader[frameIndex].isDynamic = m_SceneDataFromScene.SceneLightEnvironment.SkyLightSetting.isDynamicSky;
		m_UBSSceneDataForShader->Get()->SetData(&m_SceneDataForShader[frameIndex], sizeof(SceneDataForShader));
	}

	void SceneRender::InitSceneCompositePass()
	{
		FramebufferSpecification sceneCompositeFramebufferSpec;
		sceneCompositeFramebufferSpec.Attachments = { ImageFormat::RGBA32F };
		sceneCompositeFramebufferSpec.DebugName = "FinalColorFrameBuffer";
		sceneCompositeFramebufferSpec.Transfer = true;
		m_SceneCompositeFrameBuffer = Framebuffer::Create(sceneCompositeFramebufferSpec);
		PipelineSpecification sceneCompositePipelineSpec;
		sceneCompositePipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("FinalColor");
		sceneCompositePipelineSpec.TargetFramebuffer = m_SceneCompositeFrameBuffer;
		sceneCompositePipelineSpec.DepthTest = false;
		sceneCompositePipelineSpec.DebugName = "FinalColor";
		m_SceneCompositePipeline = Pipeline::Create(sceneCompositePipelineSpec);
		RenderPassSpecification sceneCompositePassSpec;
		sceneCompositePassSpec.Pipeline = m_SceneCompositePipeline;
		sceneCompositePassSpec.DebugName = "FinalColorPass";
		m_SceneCompositePass = RenderPass::Create(sceneCompositePassSpec);
        m_SceneCompositePass->SetInput("u_RendererData", m_UBSRenderSetting);
	}

	void SceneRender::InitSkyPass()
	{
		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::DEPTH32F };
		fbSpec.DebugName = "SkyFrameBuffer";
		fbSpec.ExistingImages[0] = m_LightPassFramebuffer->GetImage(0);
		fbSpec.ExistingImages[1] = m_PreDepthClearFramebuffer->GetDepthImage();
		fbSpec.ClearDepthOnLoad = false;
		fbSpec.ClearColorOnLoad = false;
		m_SkyFrameBuffer = Framebuffer::Create(fbSpec);
		PipelineSpecification pSpec;
		pSpec.Shader = Renderer::GetShaderLibrary()->Get("Sky");
		pSpec.TargetFramebuffer = m_SkyFrameBuffer;
		pSpec.DebugName = "SkyPipeline";
		pSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
		m_SkyPipeline = Pipeline::Create(pSpec);
		RenderPassSpecification passSpec;
		passSpec.Pipeline = m_SkyPipeline;
		passSpec.DebugName = "SkyPass";
		m_SkyPass = RenderPass::Create(passSpec);

		m_SkyPass->SetInput("u_CameraData", m_UBSCameraData);

		m_SkyPass->SetInput("u_Scene", m_UBSSceneDataForShader);
	}
	void SceneRender::SkyPass() {
		m_SkyPass->SetInput("SkyTexture", m_EnvTextures.m_EnvPreFilterMap);
		m_SkyPass->SetInput("u_SkyViewLut", m_SkyViewLutImage);
		m_SkyPass->SetInput("u_TransmittanceLut", m_TransmittanceLutImage);
		m_SkyPass->SetInput("u_MultiScatteringLut", m_MultiScatteringLutImage);

		Renderer::BeginRenderPass(m_CommandBuffer, m_SkyPass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer, 3);
		Renderer::EndRenderPass(m_CommandBuffer);
	}


	void SceneRender::preCompute()
	{
		m_CommandBuffer->Begin();
		m_EnvTextures = m_EnvPass.compute("", m_CommandBuffer);
		TransmiitanceLutPass();

		m_CommandBuffer->End();
		m_CommandBuffer->Submit();
	}

	void SceneRender::SkyViewLutPass()
	{
		m_SkyViewLutPass->SetInput("SkyViewLut",m_SkyViewLutImage);
		m_SkyViewLutPass->SetInput("u_TransmittanceLut",m_TransmittanceLutImage);
		m_SkyViewLutPass->SetInput("u_MultiScatteringLut",m_MultiScatteringLutImage);

		Renderer::BeginComputePass(m_CommandBuffer, m_SkyViewLutPass);
		Renderer::DispatchCompute(m_CommandBuffer, m_SkyViewLutPass, nullptr, glm::ivec3(SkyViewLutWidth / 8, SkyViewLutHeight / 8, 1));
		Renderer::EndComputePass(m_CommandBuffer, m_SkyViewLutPass);
	}
	void SceneRender::InitAtmospherePass()
	{
		// TransmittanceLut Pass
		TextureSpecification spec;
		spec.Width = TrasmittanceLutWidth;
		spec.Height = TrasmittanceLutHeight;
		spec.DebugName = "TransmittanceLutTexture";
		spec.Storage = true;
		spec.GenerateMips = false;
		spec.SamplerWrap = TextureWrap::Clamp;
		m_TransmittanceLutImage = Texture2D::Create(spec);

		Ref<Shader> TransmittanceLutShader = Renderer::GetShaderLibrary()->Get("TransmittanceLut");
		ComputePassSpecification TransmittanceLutSpec;
		TransmittanceLutSpec.DebugName = "TransmittanceLutPass";
		TransmittanceLutSpec.Pipeline = PipelineCompute::Create(TransmittanceLutShader);
		m_TransmittanceLutPass = ComputePass::Create(TransmittanceLutSpec);

		// MultiScatteringLut Pass
		spec.Width = MultiScatteringLutResolution;
		spec.Height = MultiScatteringLutResolution;
		spec.DebugName = "MultiScatteringLutTexture";
		spec.Storage = true;
		spec.GenerateMips = false;
		m_MultiScatteringLutImage = Texture2D::Create(spec);
		Ref<Shader> MultiScatteringLutShader = Renderer::GetShaderLibrary()->Get("MultiScatteringLut");
		ComputePassSpecification MultiScatteringLutSpec;
		MultiScatteringLutSpec.DebugName = "MultiScatteringLutPass";
		MultiScatteringLutSpec.Pipeline = PipelineCompute::Create(MultiScatteringLutShader);
		m_MultiScatteringLutPass = ComputePass::Create(MultiScatteringLutSpec);

		// SkyView Pass
		spec.Width = SkyViewLutWidth;
		spec.Height = SkyViewLutHeight;
		spec.DebugName = "SkyViewLutTexture";
		spec.Storage = true;
		spec.GenerateMips = false;
		spec.GenerateMips = false;
		m_SkyViewLutImage = Texture2D::Create(spec);
		Ref<Shader> SkyViewLutShader = Renderer::GetShaderLibrary()->Get("SkyViewLut");
		ComputePassSpecification SkyViewLutSpec;
		SkyViewLutSpec.DebugName = "SkyViewLutPass";
		SkyViewLutSpec.Pipeline = PipelineCompute::Create(SkyViewLutShader);
		m_SkyViewLutPass = ComputePass::Create(SkyViewLutSpec);
		m_SkyViewLutPass->SetInput("u_Scene", m_UBSSceneDataForShader);
		m_SkyViewLutPass->SetInput("u_CameraData", m_UBSCameraData);
	}
	void SceneRender::MultiScatteringLutPass() {
		m_MultiScatteringLutPass->SetInput("MultiScatteringLut",m_MultiScatteringLutImage);
		m_MultiScatteringLutPass->SetInput("u_TransmittanceLut",m_TransmittanceLutImage);
		Renderer::BeginComputePass(m_CommandBuffer, m_MultiScatteringLutPass);
		Renderer::DispatchCompute(m_CommandBuffer, m_MultiScatteringLutPass, nullptr, glm::ivec3(MultiScatteringLutResolution / 8, MultiScatteringLutResolution / 8, 1));
		Renderer::EndComputePass(m_CommandBuffer, m_MultiScatteringLutPass);
	}

	void SceneRender::TransmiitanceLutPass() {
		m_TransmittanceLutPass->SetInput("transmittanceLut",m_TransmittanceLutImage);
		Renderer::BeginComputePass(m_CommandBuffer, m_TransmittanceLutPass);
		Renderer::DispatchCompute(m_CommandBuffer, m_TransmittanceLutPass, nullptr, glm::ivec3(TrasmittanceLutWidth / 8, TrasmittanceLutHeight / 8, 1));
		Renderer::EndComputePass(m_CommandBuffer, m_TransmittanceLutPass);
	}


	void SceneRender::InitBloomPass()
	{
		m_BloomShader = Renderer::GetShaderLibrary()->Get("Bloom");
		ComputePassSpecification bloomPassSpecification;
		bloomPassSpecification.DebugName = "BloomPass";
		bloomPassSpecification.Pipeline = PipelineCompute::Create(m_BloomShader);
		m_BloomPass = ComputePass::Create(bloomPassSpecification);
		m_BloomPreFilterMaterial = MaterialOld::Create(m_BloomShader);
		TextureSpecification spec;
		spec.Format = ImageFormat::RGBA32F;
		spec.DebugName = "BloomImage";
		spec.SamplerWrap = TextureWrap::Clamp;  // Ϊ�˴����߽����
		spec.Storage = true;
		spec.GenerateMips = true;
		m_BloomImage = Texture2D::Create(spec);
		m_BloomImageViews.clear();
		m_BloomImageViews.resize(m_BloomImage->GetMipLevelCount());
		ImageViewSpecification imageViewSpec;
		m_BloomPreDownSamplerMaterials.clear();
		m_BloomPreUpSamplerMaterials.clear();
		m_BloomPreDownSamplerMaterials.resize(m_BloomImage->GetMipLevelCount());
		m_BloomPreUpSamplerMaterials.resize(m_BloomImage->GetMipLevelCount());
		for (int i = 0; i < m_BloomImage->GetMipLevelCount(); i++) {
			imageViewSpec.DebugName = "BloomImageView" + std::to_string(i);
			imageViewSpec.Image = m_BloomImage->GetImage();
			imageViewSpec.Mip = i;
			m_BloomImageViews[i] = ImageView::Create(imageViewSpec);
			m_BloomPreDownSamplerMaterials[i] = MaterialOld::Create(m_BloomShader);
			m_BloomPreUpSamplerMaterials[i] = MaterialOld::Create(m_BloomShader);
		}
		m_BloomPass->SetInput("u_CameraData", m_UBSCameraData);

	}
	void SceneRender::BloomPass()
	{
		uint32_t m_BloomComputeWorkgroupSize = 8;
		struct BloomComputePushConstants
		{
			glm::vec4 Params;
			float LOD = 0.0f;
			int Mode = 0; // 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
		} bloomComputePushConstants;
		struct BloomSettings
		{
			bool Enabled = true;
			float Threshold = 1.0f;
			float Knee = 0.1f;
			float UpsampleScale = 1.0f;
			float Intensity = 1.0f;
			float DirtIntensity = 1.0f;
		}m_BloomSettings;
		bloomComputePushConstants.Params = { m_BloomSettings.Threshold, m_BloomSettings.Threshold - m_BloomSettings.Knee, m_BloomSettings.Knee * 2.0f, 0.25f / m_BloomSettings.Knee };

		// ��ȡ����
		bloomComputePushConstants.Mode = 0;
		glm::uvec3 workGroups(0);
		Renderer::BeginComputePass(m_CommandBuffer, m_BloomPass);
		m_BloomPreFilterMaterial->SetInput("o_Texture", m_BloomImage);
		m_BloomPreFilterMaterial->SetInput("u_InputTexture", m_SkyFrameBuffer->GetImage(0));
		workGroups = { (m_BloomImage->GetWidth() + m_BloomComputeWorkgroupSize + 1) / m_BloomComputeWorkgroupSize, (m_BloomImage->GetHeight() + m_BloomComputeWorkgroupSize + 1) / m_BloomComputeWorkgroupSize, 1 };
		Renderer::DispatchCompute(m_CommandBuffer, m_BloomPass, m_BloomPreFilterMaterial, workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));
		m_BloomPass->GetPipeline()->ImageMemoryBarrier(m_CommandBuffer, m_BloomImage->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);

		// �²���
		bloomComputePushConstants.Mode = 1;
		uint32_t mipCount = m_BloomImage->GetMipLevelCount();
		uint32_t width = m_BloomImage->GetWidth();
		uint32_t height = m_BloomImage->GetHeight();
		for (uint32_t i = 1; i < mipCount; i++) {
			bloomComputePushConstants.LOD = i - 1;// ��ȡi-1��mip��ģ����д���i��
			m_BloomPreDownSamplerMaterials[i]->SetInput("o_Texture", m_BloomImageViews[i]); // д��i��mip
			m_BloomPreDownSamplerMaterials[i]->SetInput("u_InputTexture", m_BloomImage);
			auto [mipWidth, mipHeight] = m_BloomImage->GetMipSize(i);
			workGroups = { (uint32_t)glm::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize) ,(uint32_t)glm::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize), 1 };
			Renderer::DispatchCompute(m_CommandBuffer, m_BloomPass, m_BloomPreDownSamplerMaterials[i], workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));
			m_BloomPass->GetPipeline()->ImageMemoryBarrier(m_CommandBuffer, m_BloomImage->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
		}

		// �ϲ���
		width = 1;
		height = 1;
		bloomComputePushConstants.Mode = 3;
		mipCount = m_BloomImage->GetMipLevelCount();
		for (int i = mipCount - 2; i >= 0; i--) {
			bloomComputePushConstants.LOD = i;
			auto [mipWidth, mipHeight] = m_BloomImage->GetMipSize(i);
			workGroups.x = (uint32_t)glm::ceil((float)mipWidth / (float)m_BloomComputeWorkgroupSize);
			workGroups.y = (uint32_t)glm::ceil((float)mipHeight / (float)m_BloomComputeWorkgroupSize);
			m_BloomPreUpSamplerMaterials[i]->SetInput("o_Texture", m_BloomImageViews[i]);
			m_BloomPreUpSamplerMaterials[i]->SetInput("u_InputTexture", m_BloomImage);
			Renderer::DispatchCompute(m_CommandBuffer, m_BloomPass, m_BloomPreUpSamplerMaterials[i], workGroups, Buffer(&bloomComputePushConstants, sizeof(bloomComputePushConstants)));
			m_BloomPass->GetPipeline()->ImageMemoryBarrier(m_CommandBuffer, m_BloomImage->GetImage(), ResourceAccessFlags::ShaderWrite, ResourceAccessFlags::ShaderRead);
		}



		Renderer::EndComputePass(m_CommandBuffer, m_BloomPass);
	}

}
