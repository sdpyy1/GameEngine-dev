#pragma once

#include "Hazel/Core/UUID.h"
#include "Scene.h"
#include "Components.h"
#include "Hazel/Utils/Serializable.h"
#include "entt.hpp"

namespace GameEngine {

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene);
		Entity(entt::entity handle, std::shared_ptr<Scene> scene);
		Entity(const Entity& other) = default;
		UUID GetParentUUID() { return GetComponent<RelationshipComponent>().ParentHandle; }
		void SetParentUUID(UUID parent) { GetComponent<RelationshipComponent>().ParentHandle = parent; }
		std::vector<UUID>& Children() {
			if (!HasComponent<RelationshipComponent>())
				return std::vector<UUID>();
			return GetComponent<RelationshipComponent>().Children; 
		}
		TransformComponent& Transform() { return GetComponent<TransformComponent>(); }
		Entity Entity::GetParent()
		{
			return m_Scene->GetEntityByUUID(GetParentUUID());
		}
		bool RemoveChild(Entity child)
		{
			UUID childId = child.GetUUID();
			std::vector<UUID>& children = Children();
			auto it = std::find(children.begin(), children.end(), childId);
			if (it != children.end())
			{
				children.erase(it);
				return true;
			}

			return false;
		}

		void SetParent(Entity parent)
		{
			Entity currentParent = GetParent();
			if (currentParent == parent)
				return;
			if (currentParent)
				currentParent.RemoveChild(*this);
			SetParentUUID(parent.GetUUID());
			if (parent)
			{
				auto& parentChildren = parent.Children();
				UUID uuid = GetUUID();
				if (std::find(parentChildren.begin(), parentChildren.end(), uuid) == parentChildren.end())
					parentChildren.emplace_back(GetUUID());
			}
		}
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			ASSERT(!HasComponent<T>(), "Entity already has component!");
			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}
		template<typename T>
		T& GetComponentConst() const
		{
			ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}
		template<typename T>
		bool HasComponent() const
		{
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}
		operator bool() const { return m_EntityHandle != entt::null; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		const std::string& GetName() { return GetComponent<TagComponent>().Tag; }

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;

	private:
		friend class cereal::access;            	
		template<class Archive>                 	
		void save(Archive& ar) const
		{
			SerailizeComponent(IDComponent);
			SerailizeComponent(TagComponent);
			SerailizeComponent(TransformComponent);
			SerailizeComponent(RelationshipComponent);
			SerailizeComponent(DirectionalLightComponent);
			SerailizeComponent(PointLightComponent);
			SerailizeComponent(SpotLightComponent);
			SerailizeComponent(SkyComponent);
			SerailizeComponent(PostProcessingComponent);
			SerailizeComponent(ModelComponent);
			SerailizeComponent(SubmeshComponent);
		}
		friend class cereal::access;
		template<class Archive>
		void load(Archive& ar)
		{
			m_Scene = APP_SCENEMANAGER->GetActiveScene().get();
			m_EntityHandle = APP_SCENEMANAGER->GetActiveScene()->GetRegistry().create();

			DeserializeComponent(IDComponent);
			DeserializeComponent(TagComponent);
			DeserializeComponent(TransformComponent);
			DeserializeComponent(RelationshipComponent);
			DeserializeComponent(DirectionalLightComponent);
			DeserializeComponent(PointLightComponent);
			DeserializeComponent(SpotLightComponent);
			DeserializeComponent(SkyComponent);
			DeserializeComponent(PostProcessingComponent);
            DeserializeComponent(ModelComponent);
            DeserializeComponent(SubmeshComponent);
		}
	};

}
