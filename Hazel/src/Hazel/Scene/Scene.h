#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Scene/EditorCamera.h"
#include "entt.hpp"

class b2World;

namespace GameEngine {
	class MeshSource;
	class MeshNode;
	class SceneRender;
	class Entity;
	class Skeleton;
	struct DirLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };

		float Intensity = 1.0f;
	};

	struct DirectionalLight
	{
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float ShadowAmount = 1.0f;
	};

	struct PointLight
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
		float MinRadius = 0.001f;
		float Radius = 25.0f;
		float Falloff = 1.f;
		float SourceSize = 0.1f;
		bool CastsShadows = true;
		char Padding[3]{ 0, 0, 0 };
	};

	struct SpotLight
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Intensity = 0.0f;
		glm::vec3 Direction = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Radiance = { 0.0f, 0.0f, 0.0f };
        float Angle = 80.0f;
		float Range = 100.f;
	};
	struct RenderSettingData {
		glm::vec4 CascadeSplits;
		float LightSize = 0.5;
		int ShadowType = 2;  // 0=Hard 1=PCF 2=PCSS
		int deBugCSM = 0;
		float bloomScale = 1.0f;
	};
	struct SkyLightSetting {
		bool isDynamicSky;
		std::filesystem::path selelctEnvPath;
	};
	struct LightEnvironment
	{
		static constexpr size_t MaxDirectionalLights = 1;
		DirectionalLight DirectionalLights[MaxDirectionalLights];
		std::vector<PointLight> PointLights;
		std::vector<SpotLight> SpotLights;
		SkyLightSetting SkyLightSetting;
		[[nodiscard]] uint32_t GetPointLightsSize() const { return (uint32_t)(PointLights.size() * sizeof(PointLight)); }
		[[nodiscard]] uint32_t GetSpotLightsSize() const { return (uint32_t)(SpotLights.size() * sizeof(SpotLight)); }
	};
	struct AtmosphereParameter {
		float BottomRadius = 100;
		float TopRadius = 500;
	};
	struct SceneInfo
	{
		EditorCamera camera;
		LightEnvironment SceneLightEnvironment;
		RenderSettingData RenderSettingData;
		AtmosphereParameter AtmosphereParameter;
	};
	class Scene
	{
	public:
		Scene();
		void PackupSceneInfo(EditorCamera& editorCamera);
		~Scene();
		void testButton() { LOG_INFO("Test");};
		void ShowDebugTexture();
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
		entt::registry& GetRegistry() { return m_Registry; }
		RenderSettingData& GetRenderSettingData() { return m_SceneInfo.RenderSettingData; }
		SceneInfo& GetSceneInfo() { return m_SceneInfo; }
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
		float m_ViewportWidth = 1216.0f, m_ViewportHeight = 849.0f;
		friend class Entity;
		friend class SceneSerializer;
		friend class SceneRender;
		SceneInfo m_SceneInfo;
		std::shared_ptr<Entity> m_SelectedEntity;
	};
}
