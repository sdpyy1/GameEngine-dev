#include "hzpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Hazel/Core/Application.h"
#include "Components.h"
#include "ScriptableEntity.h"
#include "Hazel/Scene/EditorCamera.h"
#define GLM_FORCE_DEPTH_ZERO_TO_FE
#include <glm/glm.hpp>

#include <imgui.h>

namespace GameEngine {

	bool Scene::HasDirLight()
	{
		auto& views = GetAllEntitiesWith<DirectionalLightComponent>();
        return !views.empty();
	}

	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
	{
		glm::mat4 transform(1.0f);

		Entity parent = GetEntityByUUID(entity.GetParentUUID());
		if (parent)
			transform = GetWorldSpaceTransformMatrix(parent);

		return transform * entity.Transform().GetTransform();
	}
	glm::mat4 Scene::GetLocalTransformMatrix(Entity entity, const glm::mat4& worldMatrix)
	{
		glm::mat4 parentWorld(1.0f);
		Entity parent = GetEntityByUUID(entity.GetParentUUID());
		if (parent)
			parentWorld = GetWorldSpaceTransformMatrix(parent);

		// 局部矩阵 = 父矩阵逆 * 世界矩阵
		return glm::inverse(parentWorld) * worldMatrix;
	}
	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
			{
				auto view = src.view<Component>();
				for (auto srcEntity : view)
				{
					entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

					auto& srcComponent = src.get<Component>(srcEntity);
					dst.emplace_or_replace<Component>(dstEntity, srcComponent);
				}
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
			{
				if (src.HasComponent<Component>())
					dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateChildEntity({}, name);
	}

	Entity Scene::CreateChildEntity(Entity parent, const std::string& name)
	{
		auto entity = Entity{ m_Registry.create(), this };
		auto& idComponent = entity.AddComponent<IDComponent>();
		idComponent.ID = {};

		entity.AddComponent<TransformComponent>();
		if (!name.empty())
			entity.AddComponent<TagComponent>(name);

		entity.AddComponent<RelationshipComponent>();

		if (parent)
			entity.SetParent(parent);

		m_EntityIDMap[idComponent.ID] = entity;

		return entity;
	}
	void Scene::SortEntities()
	{
		m_Registry.sort<IDComponent>([&](const auto lhs, const auto rhs)
			{
				auto lhsEntity = m_EntityIDMap.find(lhs.ID);
				auto rhsEntity = m_EntityIDMap.find(rhs.ID);
				return static_cast<uint32_t>(lhsEntity->second) < static_cast<uint32_t>(rhsEntity->second);
			});
	}
	void Scene::DestroyEntity(Entity entity, bool destroyChilds)
	{
		if (destroyChilds) {
			auto uuidChilds = entity.Children();
			if (!uuidChilds.empty()) {
				for (auto child : uuidChilds) {
					Entity &c = GetEntityByUUID(child);
					if (c) {
						DestroyEntity(c, destroyChilds);
					}
				}
			}
		}
		m_EntityIDMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);

	}
	void Scene::ClearEntities()
	{
		m_Registry.each([this](entt::entity entityID) {
			Entity entity{ entityID, this };
			DestroyEntity(entity,false);  // 因为它会遍历所有的清除，不需要递归清除
			});
		m_Registry.clear();
		m_EntityIDMap.clear();
	}
	Entity Scene::DuplicateEntity(Entity entity)
	{
		// Copy name because we're going to modify component data structure
		std::string name = entity.GetName();
		Entity newEntity = CreateEntity(name);
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
		return newEntity;
	}

	Entity Scene::FindEntityByName(std::string_view name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const TagComponent& tc = view.get<TagComponent>(entity);
			if (tc.Tag == name)
				return Entity{ entity, this };
		}
		return {};
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		if (const auto iter = m_EntityIDMap.find(uuid); iter != m_EntityIDMap.end())
			return iter->second;
		return Entity{};
	}


	void Scene::OnLoadAsset()
	{
	}

	void Scene::OnSaveAsset()
	{
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(sizeof(T) == 0);
	}
	template<>
	void Scene::OnComponentAdded<ModelComponent>(Entity entity, ModelComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}


	template<>
	void Scene::OnComponentAdded<SubmeshComponent>(Entity entity, SubmeshComponent& component)
	{
	}


	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<RelationshipComponent>(Entity entity, RelationshipComponent& component)
	{
	}
	
	template<>
	void Scene::OnComponentAdded<DirectionalLightComponent>(Entity entity, DirectionalLightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpotLightComponent>(Entity entity, SpotLightComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<PointLightComponent>(Entity entity, PointLightComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<PostProcessingComponent>(Entity entity, PostProcessingComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		component.CameraRef = std::make_shared<EditorCamera>(*APP_SCENE_DEFAULT_CAMERA); // 创建一个与当前相机一模一样的相机
	}
	template<>
	void Scene::OnComponentAdded<LightProbeComponent>(Entity entity, LightProbeComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<SkyComponent>(Entity entity, SkyComponent& component)
	{
		component.iblPath.clear(); // 先清空

		std::filesystem::path assetsDir = "Assets"; // 根目录，可以根据实际路径调整

		if (std::filesystem::exists(assetsDir) && std::filesystem::is_directory(assetsDir))
		{
			for (auto& entry : std::filesystem::recursive_directory_iterator(assetsDir))
			{
				if (entry.is_regular_file())
				{
					auto ext = entry.path().extension().string();
					// 小写匹配
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
					if (ext == ".hdr")
					{
						component.iblPath.push_back(entry.path());
					}
				}
			}
		}
	}

	void Scene::LoadModel(const std::filesystem::path& path)
	{
		ModelRef model = AssetManager::DeserializeAsset<Model>(path);

		Entity modelEntity = CreateEntity(path.string());

		auto& modelComponent = modelEntity.AddComponent<ModelComponent>(model->GetUUID(), path);

		int submeshIndex = 0;
		for (auto& mesh : model->GetSubmeshes()) {
			auto& subMeshEntity = CreateChildEntity(modelEntity, mesh.mesh->name);
			subMeshEntity.AddComponent<SubmeshComponent>(model->GetUUID(), submeshIndex++);
		}
	}

	Entity Scene::GetSelectedEntity()
	{
		if (m_SelectedEntity) {
			return *m_SelectedEntity;
		}
		return {};
	}

	void Scene::SetSelectedEntity(Entity entity)
	{
		if (!entity) {
			m_SelectedEntity = nullptr;
		}
		m_SelectedEntity = std::make_shared<Entity>(entity);
	}

}
