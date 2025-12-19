#include "hzpch.h"
#include "LightCollector.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Scene/Components.h>
#include <Hazel/Scene/Entity.h>

namespace GameEngine {
	LightInfo LightCollector::s_LightInfo = {};

	void LightCollector::CollectLight()
	{
		auto& scene = APP_SCENEMANAGER->GetActiveScene();
		LightInfo lightInfo;
		{
			auto& dirLightEntity = scene->GetAllEntitiesWith<DirectionalLightComponent>();
			for (auto entity : dirLightEntity)
			{
				lightInfo.directionLightCount = 1;

				Entity curEntity = { entity, scene };
				auto& dirLightComp = curEntity.GetComponent<DirectionalLightComponent>();
				auto& transformComp = curEntity.GetComponent<TransformComponent>();

				DirectionLight dirLightInfo;
				lightInfo.dirLights.direction = glm::normalize(transformComp.GetDirection());
				lightInfo.dirLights.radiance = dirLightComp.Radiance;
				lightInfo.dirLights.intensity = dirLightComp.Intensity;
                lightInfo.dirLights.position = transformComp.Translation;
                lightInfo.dirLights.showDirection = dirLightComp.showDirection?1:0;
				// CSM
				CascadeData cascades[CSM_LEVEL_COUNT];
				CalculateCascades(cascades, APP_SCENE_CAMERA, lightInfo.dirLights.direction);
				for (size_t i = 0; i < CSM_LEVEL_COUNT; i++)
				{
					lightInfo.dirLights.projection[i] = cascades[i].Projection;
					lightInfo.dirLights.view[i] = cascades[i].View;
					lightInfo.dirLights.viewProj[i] = cascades[i].ViewProj;
					lightInfo.dirLights.SplitDepth[i] = cascades[i].SplitDepth;
					lightInfo.dirLights.frustum[i] = CreateFrustumFromMatrix(cascades[i].ViewProj);
				}
				break; // only one directional light
			}
			if (lightInfo.directionLightCount == 0) {
				lightInfo.dirLights.radiance = { 0.0f, 0.0f, 0.0f }; // 防止切换场景时Buffer还是上个场景的数据
			}
		}

		{
			uint32_t pointLightCount = 0;
			auto pointLightEntity = scene->GetAllEntitiesWith<PointLightComponent>();
			for (auto entity : pointLightEntity)
			{
				Entity curEntity = { entity, scene };

				auto& pointLightComp = curEntity.GetComponent<PointLightComponent>();
				auto& transformComp = curEntity.GetComponent<TransformComponent>();

				lightInfo.pointLights[pointLightCount].position = transformComp.Translation;
				lightInfo.pointLights[pointLightCount].radiance = pointLightComp.Radiance;
				lightInfo.pointLights[pointLightCount].intensity = pointLightComp.Intensity;
				lightInfo.pointLights[pointLightCount].sphere = { lightInfo.pointLights[pointLightCount].position,pointLightComp.Radius };
                lightInfo.pointLights[pointLightCount].showRadius = pointLightComp.showRadius? 1:0;

				// 6个面的view proj
				auto & position = transformComp.Translation;
				glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f); 
				lightInfo.pointLights[pointLightCount].view[0] = glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));  // +X
				lightInfo.pointLights[pointLightCount].view[1] = glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)); // -X
				lightInfo.pointLights[pointLightCount].view[2] = glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // +Y 
				lightInfo.pointLights[pointLightCount].view[3] = glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)); // -Y 
				lightInfo.pointLights[pointLightCount].view[4] = glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));  // +Z 
				lightInfo.pointLights[pointLightCount].view[5] = glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)); // -Z 
				constexpr float fov = glm::radians(90.0f);
				float aspect = 1.0f;
				float nearPlane = 0.1;
				float farPlane = lightInfo.pointLights[pointLightCount].sphere.radius;
				lightInfo.pointLights[pointLightCount].projection = glm::perspective(fov,aspect,nearPlane,farPlane );
                lightInfo.pointLights[pointLightCount].viewProj[0] = lightInfo.pointLights[pointLightCount].projection * lightInfo.pointLights[pointLightCount].view[0];
                lightInfo.pointLights[pointLightCount].viewProj[1] = lightInfo.pointLights[pointLightCount].projection * lightInfo.pointLights[pointLightCount].view[1];
                lightInfo.pointLights[pointLightCount].viewProj[2] = lightInfo.pointLights[pointLightCount].projection * lightInfo.pointLights[pointLightCount].view[2];
                lightInfo.pointLights[pointLightCount].viewProj[3] = lightInfo.pointLights[pointLightCount].projection * lightInfo.pointLights[pointLightCount].view[3];
                lightInfo.pointLights[pointLightCount].viewProj[4] = lightInfo.pointLights[pointLightCount].projection * lightInfo.pointLights[pointLightCount].view[4];
                lightInfo.pointLights[pointLightCount].viewProj[5] = lightInfo.pointLights[pointLightCount].projection * lightInfo.pointLights[pointLightCount].view[5];

				pointLightCount++;
			}
			lightInfo.pointLightCount = pointLightCount;
			if (pointLightCount == 0) {
				lightInfo.pointLights[0].radiance = { 0.0f, 0.0f, 0.0f }; // 防止切换场景时Buffer还是上个场景的数据
			}
		}

		{
			uint32_t spotLightCount = 0;
			auto spotLightEntity = scene->GetAllEntitiesWith<SpotLightComponent>();
			for (auto entity : spotLightEntity)
			{
				Entity curEntity = { entity, scene };
                auto& transformComp = curEntity.GetComponent<TransformComponent>();
				auto& spotLightComp = curEntity.GetComponent<SpotLightComponent>();
				lightInfo.spotLights[spotLightCount].direction = glm::normalize(transformComp.GetDirection());
				lightInfo.spotLights[spotLightCount].position = curEntity.GetComponent<TransformComponent>().Translation;
				lightInfo.spotLights[spotLightCount].radiance = spotLightComp.Radiance;
				lightInfo.spotLights[spotLightCount].intensity = spotLightComp.Intensity;
				lightInfo.spotLights[spotLightCount].showRange = spotLightComp.showRadius?1:0;
				lightInfo.spotLights[spotLightCount].range = spotLightComp.range;
				lightInfo.spotLights[spotLightCount].angle = spotLightComp.angle;
				lightInfo.spotLights[spotLightCount].falloff = spotLightComp.falloff;
				lightInfo.spotLights[spotLightCount].showDirection = spotLightComp.showDirection?1:0;

				spotLightCount++;
			}
			lightInfo.spotLightCount = spotLightCount;
			if (spotLightCount == 0) {
				lightInfo.spotLights[0].radiance = { 0.0f, 0.0f, 0.0f }; // 防止切换场景时Buffer还是上个场景的数据
			}
		}
		lightInfo.clusterAtomicOffset = 0; // 初始化原子计数器
		s_LightInfo = lightInfo;
		RENDER_RESOURCEMANAGER->SetLightInfo(lightInfo);
	}

	void LightCollector::CalculateCascades(CascadeData* cascades, std::shared_ptr<EditorCamera> sceneCamera, const glm::vec3& lightDirection)
	{
		float nearOffset = -250.f;
		float farOffset = 0.f;
		glm::mat4 viewProjection = sceneCamera->GetViewProjection();
		float CascadeSplitLambda = 0.9f;
		const int SHADOW_MAP_CASCADE_COUNT = CSM_LEVEL_COUNT;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
		float nearClip = sceneCamera->GetNearClip();
		float farClip = sceneCamera->GetFarClip();
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
			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDirection * radius, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(-radius, radius, -radius, radius, 0.0f + nearOffset, radius * 2 + farOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = 4096.0f; // TODO：Define in config

			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange);
			cascades[i].ViewProj = shadowMatrix;
			cascades[i].View = lightViewMatrix;
			cascades[i].Projection = lightOrthoMatrix;

			lastSplitDist = cascadeSplits[i];
		}

	}

}


