#pragma once

#include "Hazel/Core/UUID.h"


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Hazel/Math/Math.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Renderer/RenderResource/RenderStruct.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Renderer/RenderResource/RenderBuffer.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"

namespace GameEngine {

	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};


	// TODO:学 CLUSTER 和 VIRTUAL_MESH
	enum MeshRendererMode
	{
		RENDER_MODE_DEFAULT = 0,
		RENDER_MODE_CLUSTER,
		RENDER_MODE_VIRTUAL_MESH,

		RENDER_MODE_MAX,//
	};
	struct ModelComponent
	{
		UUID ModelID = 0;
		ModelRef model;
		std::filesystem::path path = "";

		bool Visible = true;
		bool isDynamic = false;
		MeshRendererMode renderMode = RENDER_MODE_DEFAULT;
		std::vector<MaterialRef> materials;
		bool castShadow = true;
		ModelComponent() = default;
		ModelComponent(UUID staticMesh, std::filesystem::path filePath)
			: ModelID(staticMesh),path(filePath){
			model = AssetManager::GetAssetByAssetHandle<Model>(staticMesh);
			isDynamic = model->hasBone();
			materials = model->GetMaterials();   // TODO:注意这个可能导致所有实例都引用同一个材质
		}
	};

	struct SubmeshComponent
	{
		UUID Mesh;
		uint32_t SubmeshIndex = 0;
		bool Visible = true;
		std::vector<UUID> BoneEntityIds;
		ModelRef model;
        MaterialRef material;
		bool castShadow = true;
		MeshInstanceInfo meshInfo;
		uint32_t meshInfoID = 0;
		glm::mat4 prevModel = glm::mat4(0);

		SubmeshComponent() = default;
		void updateMeshInfo() {
			RENDER_RESOURCEMANAGER->SetMeshInstanceInfo(meshInfo, meshInfoID);
		}
		SubmeshComponent(UUID mesh, uint32_t submeshIndex = 0)
			: Mesh(mesh), SubmeshIndex(submeshIndex)
		{
            model = AssetManager::GetAssetByAssetHandle<Model>(mesh);
            material = model->GetMaterials()[SubmeshIndex];
			castShadow = material->castShadow;
			meshInfoID = RENDER_RESOURCEMANAGER->AllocateMeshInstanceInfoID();
			meshInfo.animationID = 0;
			meshInfo.indexID = model->GetSubmeshes()[SubmeshIndex].indexBuffer->indexID;
			meshInfo.vertexID = model->GetSubmeshes()[SubmeshIndex].vertexBuffer->vertexID;
			meshInfo.materialID = model->GetMaterials()[SubmeshIndex] ? model->GetMaterials()[SubmeshIndex]->GetMaterialID() : 0;

			RENDER_RESOURCEMANAGER->SetMeshInstanceInfo(meshInfo, meshInfoID);
		}

		MeshRef GetMesh() {
			return model->GetSubMesh(SubmeshIndex);
		}
	};


	struct DynamicModelComponent
	{
		UUID meshSource = 0;
		std::filesystem::path path = "";
		bool Visible = true;

		DynamicModelComponent() = default;
		DynamicModelComponent(UUID handle, std::filesystem::path filePath) {
			path = filePath;
		}
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
	};
	struct AnimationComponent
	{
		UUID meshSource = 0;
		std::filesystem::path path;
		// const Animation* CurrentAnimation = nullptr; 
		float CurrentTime = 0.0f;
		bool IsLooping = true; 
		// Pose CurrentPose;      
		int SelectedAnimIndex = 0;
		std::vector<UUID> BoneEntityIds; 
		AnimationComponent() = default;
		AnimationComponent(UUID mesh, std::filesystem::path filePath) :meshSource(mesh), path(filePath)
		{
		}
	};
	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
	private:
		glm::vec3 RotationEuler = { 0.0f, 0.0f, 0.0f };
		glm::quat Rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

	public:
		TransformComponent() = default;
		TransformComponent(const TransformComponent& other) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation)
		{
		}

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
		bool showDirection = false;
		bool showCSM = false;
	};


	struct PointLightComponent
	{
		glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		float Radius = 1.0f;
		bool showRadius = false;
	};

	struct SpotLightComponent
	{ 
        glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
        float Intensity = 1.0f;
		float range = 1.0f;
		bool showRadius = false;
	};

	struct PostProcessingComponent
	{
		float bloomScale = 0.3f;
	};

	struct SkyComponent {
		bool DynamicSky = false;
		std::vector<std::filesystem::path> iblPath;
		int selectedIBL = 0;
		float IBLScale = 1.0f;
	};

	struct SpriteRendererComponent
	{
		
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.005f;

		CircleRendererComponent() = default;
		CircleRendererComponent(const CircleRendererComponent&) = default;
	};

	struct CameraComponent
	{
		EditorCamera Camera;
		bool Primary = true; 

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};
	struct ScriptComponent
	{
		std::string ClassName;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;
	};

	// Forward declaration
	class ScriptableEntity;

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity*(*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	// Physics

	struct Rigidbody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		// Storage for runtime
		void* RuntimeBody = nullptr;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
	};

	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.5f, 0.5f };

		// TODO(Yan): move into physics material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 0.5f;

		// TODO(Yan): move into physics material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};

	struct TextComponent
	{
		
	};

	template<typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents = 
		ComponentGroup<ModelComponent,TransformComponent,PointLightComponent,
			CircleRendererComponent, CameraComponent, ScriptComponent, SpotLightComponent,
			NativeScriptComponent, Rigidbody2DComponent, BoxCollider2DComponent, SkyComponent,
			CircleCollider2DComponent, RelationshipComponent, DirectionalLightComponent, SubmeshComponent,DynamicModelComponent, AnimationComponent, PostProcessingComponent>;

}
