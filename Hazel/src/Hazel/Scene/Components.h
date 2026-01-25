#pragma once

#include "Hazel/Core/UUID.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Hazel/Math/Math.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "Hazel/Utils/Serializable.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Renderer/RenderResource/RenderStruct.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"
#include "Hazel/Renderer/RenderResource/RenderBuffer.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"

namespace GameEngine {
	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(UUID id) :ID(id){};
		IDComponent(const IDComponent&) = default;

		BeginSerailize
			SerailizeEntry(ID)
        EndSerailize
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {
		}
		BeginSerailize
			SerailizeEntry(Tag)
		EndSerailize
	};

	struct ModelComponent
	{
		UUID ModelID = 0;
		bool Visible = true;
		std::string path = "";
		bool castShadow = true;

		ModelRef model;

		ModelComponent() = default;
		ModelComponent(UUID uuid, std::filesystem::path filePath)
			: ModelID(uuid), path(filePath.string()) {
			model = AssetManager::GetAsset<Model>(uuid);
		}
		BeginSerailize
			SerailizeEntry(ModelID)
			SerailizeEntry(path)
			model = AssetManager::GetAsset<Model>(ModelID);		
			SerailizeEntry(Visible)
			SerailizeEntry(castShadow)
		EndSerailize
	};

	struct SubmeshComponent
	{
		UUID modelID;
		uint32_t SubmeshIndex = 0;
		bool Visible = true;
		bool castShadow = true;
		std::string path;
		MeshInstanceInfo meshInfo;

		ModelRef model;
		MaterialRef material;

		glm::mat4 prevModel = glm::mat4(0);
		uint32_t meshInfoID = 0;
		SubmeshComponent() = default;
		SubmeshComponent(UUID modelID, uint32_t submeshIndex = 0)
			: modelID(modelID), SubmeshIndex(submeshIndex)
		{
			model = AssetManager::GetAsset<Model>(modelID);
			auto originalMaterial = model->GetMaterial(SubmeshIndex);
			material = model->GetMaterial(SubmeshIndex)->Clone(); // 从Model的材质模板中clone一份模板实例
			path = model->GetPath();
		}
		MeshRef GetMesh() {
			return model->GetSubMesh(SubmeshIndex);
		}
		MaterialRef GetMaterial() {
			return material;
		}
        BeginSerailize
			SerailizeEntry(modelID)
			SerailizeEntry(SubmeshIndex)
			SerailizeEntry(castShadow)
			SerailizeEntry(path)
			SerailizeEntry(Visible)
			model = AssetManager::GetAsset<Model>(modelID);	
			SerailizeEntry(material)
		EndSerailize
	};

	struct RelationshipComponent
	{
		UUID ParentHandle = 0;
		std::vector<UUID> Children;

		RelationshipComponent() = default;
		RelationshipComponent(const RelationshipComponent& other) = default;
		RelationshipComponent(UUID parent)
			: ParentHandle(parent) {
		}
        BeginSerailize
			SerailizeEntry(ParentHandle)
			SerailizeEntry(Children)
		EndSerailize
	};

	
	struct LightProbeComponent
	{ 
		bool enable = true;
		glm::uvec3 probeCount = glm::uvec3(8,8,8);
		glm::vec3 gridStep = glm::vec3(3.0f, 3.0f, 3.0f);
		uint32_t raysPerProbe = 256;
		bool visulaize = false;
		bool infiniteBounds = true;
		bool getSkyLight = true;

        BeginSerailize
			SerailizeEntry(enable)
			SerailizeEntry(probeCount)
			SerailizeEntry(gridStep)
			SerailizeEntry(raysPerProbe)
			SerailizeEntry(visulaize)
			SerailizeEntry(infiniteBounds)
			SerailizeEntry(getSkyLight)
        EndSerailize
	};


	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
	private:
		glm::vec3 RotationEuler = { 0.0f, 0.0f, 0.0f };
		glm::quat Rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		template<class Archive> void serialize(Archive& ar, glm::vec3& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z)); }
		template<class Archive> void serialize(Archive& ar, glm::quat& e) { ar(cereal::make_nvp("x", e.x), cereal::make_nvp("y", e.y), cereal::make_nvp("z", e.z), cereal::make_nvp("w", e.w)); }

        BeginSerailize
			SerailizeEntry(Translation)
			SerailizeEntry(Scale)
			SerailizeEntry(RotationEuler)
			SerailizeEntry(Rotation)
		EndSerailize
	public:
		TransformComponent() = default;
		TransformComponent(const TransformComponent& other) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation)
		{
		}

		// 缩放-> 旋转-> 平移
		glm::mat4 GetTransform() const
		{
			return glm::translate(glm::mat4(1.0f), Translation)
				* glm::toMat4(Rotation)
				* glm::scale(glm::mat4(1.0f), Scale);
		}

		void SetTransform(const glm::mat4& transform)
		{
			Math::DecomposeTransform(transform, Translation, Rotation, Scale);
			RotationEuler = glm::eulerAngles(Rotation);
		}

		glm::vec3 GetRotationEuler() const
		{
			return RotationEuler;
		}

		void SetRotationEuler(const glm::vec3& euler)
		{
			RotationEuler = euler;
			Rotation = glm::quat(RotationEuler);
		}

		glm::quat GetRotation() const
		{
			return Rotation;
		}

		void SetRotation(const glm::quat& quat)
		{
			// wrap given euler angles to range [-pi, pi]
			auto wrapToPi = [](glm::vec3 v)
				{
					return glm::mod(v + glm::pi<float>(), 2.0f * glm::pi<float>()) - glm::pi<float>();
				};

			auto originalEuler = RotationEuler;
			Rotation = quat;
			RotationEuler = glm::eulerAngles(Rotation);

			// A given quat can be represented by many Euler angles (technically infinitely many),
			// and glm::eulerAngles() can only give us one of them which may or may not be the one we want.
			// Here we have a look at some likely alternatives and pick the one that is closest to the original Euler angles.
			// This is an attempt to avoid sudden 180deg flips in the Euler angles when we SetRotation(quat).

			glm::vec3 alternate1 = { RotationEuler.x - glm::pi<float>(), glm::pi<float>() - RotationEuler.y, RotationEuler.z - glm::pi<float>() };
			glm::vec3 alternate2 = { RotationEuler.x + glm::pi<float>(), glm::pi<float>() - RotationEuler.y, RotationEuler.z - glm::pi<float>() };
			glm::vec3 alternate3 = { RotationEuler.x + glm::pi<float>(), glm::pi<float>() - RotationEuler.y, RotationEuler.z + glm::pi<float>() };
			glm::vec3 alternate4 = { RotationEuler.x - glm::pi<float>(), glm::pi<float>() - RotationEuler.y, RotationEuler.z + glm::pi<float>() };

			// We pick the alternative that is closest to the original value.
			float distance0 = glm::length2(wrapToPi(RotationEuler - originalEuler));
			float distance1 = glm::length2(wrapToPi(alternate1 - originalEuler));
			float distance2 = glm::length2(wrapToPi(alternate2 - originalEuler));
			float distance3 = glm::length2(wrapToPi(alternate3 - originalEuler));
			float distance4 = glm::length2(wrapToPi(alternate4 - originalEuler));

			float best = distance0;
			if (distance1 < best)
			{
				best = distance1;
				RotationEuler = alternate1;
			}
			if (distance2 < best)
			{
				best = distance2;
				RotationEuler = alternate2;
			}
			if (distance3 < best)
			{
				best = distance3;
				RotationEuler = alternate3;
			}
			if (distance4 < best)
			{
				best = distance4;
				RotationEuler = alternate4;
			}

			RotationEuler = wrapToPi(RotationEuler);
		}
		glm::vec3 GetDirection() const
		{
			// 默认光朝 -x
			return Rotation * glm::vec3(-1.0f, 0.0f, 0.0f);
		}
		friend class SceneSerializer;
	};

	struct DirectionalLightComponent
	{
		glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		ShadowType shadowType = SHADOW_TYPE_PCSS;
		bool showDirection = true;
		bool showCSM = false;
		bool CSMSmooth = false;
        BeginSerailize
			SerailizeEntry(Intensity)
			SerailizeEntry(Radiance)
			SerailizeEntry(shadowType)
			SerailizeEntry(showDirection)
			SerailizeEntry(showCSM)
			//SerailizeEntry(CSMSmooth)  TODO: 目前序列化如果序列之前的场景，不及时保存新的字段，再次启动会报错，待修复
		EndSerailize
	};

	struct PointLightComponent
	{
		glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		float Radius = 20.0f;
		bool showRadius = false;
        BeginSerailize
			SerailizeEntry(Intensity)
			SerailizeEntry(Radiance)
			SerailizeEntry(Radius)
			SerailizeEntry(showRadius)
		EndSerailize
	};

	struct SpotLightComponent
	{
		glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		float range = 1.0f;
		bool showRadius = false;
		float angle = 60;
		float falloff = 1.0;
		bool showDirection = true;
        BeginSerailize
			SerailizeEntry(Intensity)
			SerailizeEntry(Radiance)
			SerailizeEntry(range)
			SerailizeEntry(showRadius)
			SerailizeEntry(angle)
			SerailizeEntry(falloff)
			SerailizeEntry(showDirection)
		EndSerailize
	};

	struct PostProcessingComponent
	{
		// Bloom
		bool enableBloom = true;
		float bloomScale = 0.3f;

		// TAA
		bool enableTAA = true;
		bool taaSharpen = false;
		float taaSharpness = 1.0f;
		// FXAA
		bool enableFXAA = false;
		bool showEdge = false;

		// PathTracing
		bool pathTracingEnable = true;
		int pathTracingNumSamples = 1;
		int pathTracingTotalNumSamples;
		int pathTracingNumBounce = 5;
		bool pathTracingSampleSkyBox = true;
		bool pathTracingIndirectOnly = true;
		bool pathTracingHistoryActive = false;

		// Color
		float exposure = 0.4;
		float saturation = 1.f;
		float contrast = 1.f;
		uint32_t toneMappingMode = 0;

        BeginSerailize
			SerailizeEntry(enableBloom)
			SerailizeEntry(bloomScale)
			SerailizeEntry(enableTAA)
			// SerailizeEntry(enableFXAA)
			// SerailizeEntry(showEdge)
			SerailizeEntry(taaSharpen)
			SerailizeEntry(taaSharpness)
            SerailizeEntry(pathTracingEnable)
            SerailizeEntry(pathTracingNumSamples)
            SerailizeEntry(pathTracingNumBounce)
            SerailizeEntry(pathTracingTotalNumSamples)
            SerailizeEntry(pathTracingSampleSkyBox)
            SerailizeEntry(pathTracingIndirectOnly)
            //SerailizeEntry(pathTracingHistoryActive)  TODO:有一些逻辑问题导致新添加字段后报错了，可能原因是添加新字段后保存了一次场景，但没写序列化逻辑
		EndSerailize
	};

	struct SkyComponent {
		bool DynamicSky = false;
		std::vector<std::filesystem::path> iblPath;
		int selectedIBL = 0;
		float IBLScale = 1.0f;
        BeginSerailize
			SerailizeEntry(DynamicSky)
			SerailizeEntry(selectedIBL)
			SerailizeEntry(IBLScale)
		EndSerailize
	};

	// TODO: 这个没有序列化，因为摄像机的序列化还没写
	struct CameraComponent
	{ 
		bool Primary = false;
		EditorCameraRef CameraRef;
	};

	template<typename... Components>
	struct ComponentGroup {};

	using AllComponents = ComponentGroup<
		IDComponent,
		TagComponent,
		ModelComponent,
		TransformComponent,
		PointLightComponent,
		SpotLightComponent,
		SkyComponent,
		RelationshipComponent,
		DirectionalLightComponent,
		SubmeshComponent,
		PostProcessingComponent,
		LightProbeComponent,
		CameraComponent
	>;
}
