#pragma once
#include "entt.hpp"
#include "Hazel/Asset/Asset.h"
namespace GameEngine {
	class Entity;
	class Scene : public Asset
	{
	public:
		Scene();
		~Scene();
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
        // ================= 组件序列化/反序列化 =================

        template<class Archive>
        void SerializeEntityComponents(Archive& ar, Entity& entity)
        {
            bool hasTransform = entity.HasComponent<TransformComponent>();
            ar(cereal::make_nvp("HasTransform", hasTransform));  // 给名字
            if (hasTransform)
                ar(cereal::make_nvp("TransformComponent", entity.GetComponent<TransformComponent>()));
        }

        template<class Archive>
        void DeserializeEntityComponents(Archive& ar, Entity& entity)
        {
            bool hasTransform;
            ar(cereal::make_nvp("HasTransform", hasTransform));
            if (hasTransform)
            {
                TransformComponent comp;
                ar(cereal::make_nvp("TransformComponent", comp));
                entity.AddOrReplaceComponent<TransformComponent>(comp);
            }
        }
        friend class cereal::access;
        template<class Archive>
        void serialize(Archive& ar)
        {
            if constexpr (Archive::is_saving::value)
            {
                size_t entityCount = m_EntityIDMap.size();
                ar(cereal::make_nvp("entityCount", entityCount));
                for (auto& [uuid, entity] : m_EntityIDMap)
                {
                    IDComponent& idComp = entity.GetComponent<IDComponent>();
                    TagComponent& tagComp = entity.GetComponent<TagComponent>();
                    RelationshipComponent& relComp = entity.GetComponent<RelationshipComponent>();

                    ar(cereal::make_nvp("ID", idComp.ID),
                        cereal::make_nvp("Name", tagComp.Tag),
                        cereal::make_nvp("Parent", relComp.ParentHandle));
                    // 序列化其他组件
                    SerializeEntityComponents(ar, entity);
                }
            }
            else if constexpr (Archive::is_loading::value)
            {
                size_t entityCount;
                ar(entityCount);

                std::vector<std::tuple<Entity, UUID>> tempEntities;
                tempEntities.reserve(entityCount);
                for (size_t i = 0; i < entityCount; ++i)
                {
                    UUID uuid;
                    std::string name;
                    UUID parentUUID;
                    ar(cereal::make_nvp("ID", uuid),
                        cereal::make_nvp("Name", name),
                        cereal::make_nvp("Parent", parentUUID));
                    Entity entity = CreateEntity(name);

                    // 重置ID组件
                    entity.AddOrReplaceComponent<IDComponent>(uuid);

                    // 添加RelationshipComponent
                    entity.SetParentUUID(parentUUID);

                    tempEntities.push_back({ entity, uuid });
                }

                //for (auto& [entity, uuid] : tempEntities)
                //{
                //    RelationshipComponent& rel = entity.GetComponent<RelationshipComponent>();
                //    if (rel.ParentHandle != 0)
                //    {
                //        Entity parent = GetEntityByUUID(rel.ParentHandle);
                //        if (parent)
                //            entity.SetParent(parent);
                //    }
                //}

                for (auto& [entity, uuid] : tempEntities)
                {
                    DeserializeEntityComponents(ar, entity);
                }
            }
        }

        



	};
	using SceneRef =  std::shared_ptr<Scene> ;
}
