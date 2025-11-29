#include "hzpch.h"
#include "Entity.h"

namespace GameEngine {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}


	Entity::Entity(entt::entity handle, std::shared_ptr<Scene> scene): m_EntityHandle(handle), m_Scene(scene.get())
	{

	}

}
