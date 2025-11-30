#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/Scene/EditorCamera.h"
#include "entt.hpp"

namespace GameEngine {
	class Entity;
	class Scene
	{
	public:
		Scene();
		~Scene();
		void testButton() { LOG_INFO("Test");};
		void SetViewprotSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; }
		void LoadModel(const std::filesystem::path& path);
		Entity GetSelectedEntity();
		void SetSelectedEntity(Entity entity);


	public:
		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateChildEntity(Entity parent, const std::string& name);
		void SortEntities();
		void DestroyEntity(Entity entity, bool destroyChilds = true);
		Entity DuplicateEntity(Entity entity);
		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);
		void ClearEntities();
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		template<typename... Components>
		auto GetFirstEntityWith()
		{
			auto view = m_Registry.view<Components...>();
            return view.front();
		}
		entt::registry& GetRegistry() { return m_Registry; }
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		bool HasDirLight();
		glm::mat4 GetLocalTransformMatrix(Entity entity, const glm::mat4& worldMatrix);
	private:
		Entity TryGetDescendantEntityWithTag(Entity entity, const std::string& tag);
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	private:
		entt::registry m_Registry;
		using EntityMap = std::unordered_map<UUID, Entity>;
		EntityMap m_EntityIDMap;
		float m_ViewportWidth, m_ViewportHeight;
		std::shared_ptr<Entity> m_SelectedEntity;
		friend class Entity;
		friend class SceneSerializer;
	};
	using SceneRef =  std::shared_ptr<Scene> ;
}
