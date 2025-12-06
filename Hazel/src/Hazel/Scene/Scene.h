#pragma once
#include "entt.hpp"
#include "Hazel/Asset/Asset.h"
namespace GameEngine {
	class Entity;
	class Scene : public Asset
	{
	public:
		Scene() = default;
		~Scene() = default;
		virtual std::string GetAssetTypeName() override { return "Asset_Scene"; }
		virtual AssetType GetAssetType() override { return ASSET_TYPE_SCENE; }
		virtual void OnLoadAsset() override;
		virtual void OnSaveAsset() override;

	public:
		void testButton() { LOG_INFO("Test"); };
		void SetViewprotSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; }
		void LoadModel(const std::filesystem::path& path);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		bool HasDirLight();
		glm::mat4 GetLocalTransformMatrix(Entity entity, const glm::mat4& worldMatrix);

	public:
		Entity GetSelectedEntity();
		void SetSelectedEntity(Entity entity);

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateChildEntity(Entity parent, const std::string& name);
		void SortEntities();
		void DestroyEntity(Entity entity, bool destroyChilds = true);
		Entity DuplicateEntity(Entity entity);
		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);
		void ClearEntities();
		template<typename Func>
		void ForEachEntity(Func&& func)
		{
			for (auto& [uuid, entity] : m_EntityIDMap)
			{
				func(entity);
			}
		}
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

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

	private:
		entt::registry m_Registry;
		std::unordered_map<UUID, Entity> m_EntityIDMap;
		float m_ViewportWidth, m_ViewportHeight;
		std::shared_ptr<Entity> m_SelectedEntity;
		friend class Entity;
		friend class SceneSerializer;

	private:
        friend class cereal::access;
        template<class Archive>
        void serialize(Archive& ar)
        {
            if constexpr (Archive::is_saving::value)
            {
                ar(cereal::make_nvp("EntityCount", m_EntityIDMap.size())); 
                for (auto& [uuid, entity] : m_EntityIDMap)
                {
                    ar(cereal::make_nvp("Entity", entity));
                }
            }
            else {
                m_EntityIDMap.clear();
                size_t entityCount = 0;
                ar(cereal::make_nvp("EntityCount", entityCount));
                for (size_t i = 0; i < entityCount; ++i) {
                    Entity entity;
                    ar(cereal::make_nvp("Entity", entity));
                    m_EntityIDMap[entity.GetUUID()] = entity;
                }
            }
            
        }

        



	};
	using SceneRef =  std::shared_ptr<Scene> ;
}
