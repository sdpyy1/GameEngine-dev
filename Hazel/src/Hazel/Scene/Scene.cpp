#include "hzpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Hazel/Core/Application.h"
#include "Components.h"
#include "ScriptableEntity.h"
#include "Hazel/Renderer/old/Renderer.h"
#define GLM_FORCE_DEPTH_ZERO_TO_FE
#include <glm/glm.hpp>

#include "Hazel/Utils/UIUtils.h"
#include "Hazel/Renderer/old/RendererManager.h"
#include <imgui.h>
#include <Hazel/Asset/AssetImporter.h>
#include <Hazel/Asset/Model/Mesh.h>
#include "SceneRender.h"

namespace GameEngine {
	Scene::Scene()
	{
	}
	void Scene::ShowDebugTexture()
	{
		// UI::Image(Application::GetRendererManager()->GetTextureWhichNeedDebug(), ImGui::GetContentRegionAvail(), {0, 0}, {1, 1});
	}
	bool Scene::HasDirLight()
	{
		auto& views = GetAllEntitiesWith<DirectionalLightComponent>();
        return !views.empty();
	}
	// 打包一帧的场景数据
	void Scene::PackupSceneInfo(EditorCamera& editorCamera) {
		m_SceneInfo.camera = editorCamera;
		auto& light = m_SceneInfo.SceneLightEnvironment;
		light = LightEnvironment{};
		// Directional Lights
		{
			auto lights = m_Registry.group<DirectionalLightComponent>(entt::get<TransformComponent>);
			uint32_t directionalLightIndex = 0;
			for (auto entity : lights)
			{
				auto [transformComponent, lightComponent] = lights.get<TransformComponent, DirectionalLightComponent>(entity);

				glm::vec3 direction = glm::normalize(-transformComponent.Translation);

				ASSERT(directionalLightIndex < LightEnvironment::MaxDirectionalLights);
				light.DirectionalLights[directionalLightIndex++] =
				{
					direction,
					lightComponent.Intensity,
					lightComponent.Radiance,
					lightComponent.ShadowAmount,	
				};
			}
		}
		// Spot Lights
        {
            auto lights = m_Registry.group<SpotLightComponent>(entt::get<TransformComponent>);
            uint32_t spotLightIndex = 0;
			light.SpotLights.resize(lights.size());
            for (auto entity : lights)
            {
                auto [transformComponent, lightComponent] = lights.get<TransformComponent, SpotLightComponent>(entity);
                glm::vec3 direction = glm::normalize(transformComponent.GetDirection());
                light.SpotLights[spotLightIndex++] =
                {
					transformComponent.Translation,
                    lightComponent.Intensity,
					direction,
                    lightComponent.Radiance,
                };
            }
        }

		auto skyLight = GetAllEntitiesWith<SkyComponent>();
        if (!skyLight.empty()) {
            auto& skyComponent = skyLight.get<SkyComponent>(skyLight[0]);
			light.SkyLightSetting = {
				skyComponent.DynamicSky,
				skyComponent.iblPath[skyComponent.selectedIBL]
			};
			m_SceneInfo.RenderSettingData.bloomScale = skyComponent.bloomScale;
        }
	}


	void Scene::UpdateAnimation(Timestep ts) {
		auto view = GetAllEntitiesWith<AnimationComponent>();
		for (auto e : view) {
			Entity entity = { e, this };
			auto& animComp = entity.GetComponent<AnimationComponent>();

			if (animComp.meshSource == 0 || animComp.BoneEntityIds.empty() || !animComp.CurrentAnimation)
				continue;

			const auto& animation = animComp.CurrentAnimation;
			float duration = animation->GetDuration();

			if (ts > 0.0f) {
				animComp.CurrentTime += ts;
				if (animComp.IsLooping)
					animComp.CurrentTime = std::fmod(animComp.CurrentTime, duration);
				else
					animComp.CurrentTime = std::clamp(animComp.CurrentTime, 0.0f, duration);

				animation->Sample(animComp.CurrentTime, animComp.CurrentPose);

				for (size_t i = 0; i < animComp.BoneEntityIds.size(); ++i) {
					Entity boneEntity = GetEntityByUUID(animComp.BoneEntityIds[i]);
					if (!boneEntity.HasComponent<TransformComponent>())
						continue;
					auto& boneTransform = boneEntity.GetComponent<TransformComponent>();
					const auto& sampled = animComp.CurrentPose.BoneTransforms[i == 0 ? 0 : i + 1];  // ？？？ 这应该和设计有关系

					boneTransform.Translation = sampled.Translation;
					boneTransform.SetRotation(sampled.Rotation);
					boneTransform.Scale = sampled.Scale;
				}
			}
		}
	}

	void Scene::OutputViewport()
	{
		IconData iconData;
		iconData.LoadIconData("Assets/Texture/texture.jpg",false);
		//UI::Image(Application::GetRendererManager()->GetFinalImage(), ImGui::GetContentRegionAvail(), { 0, 0 }, { 1, 1 });
		ImGui::Image(iconData.textureID->RawHandle(), ImGui::GetContentRegionAvail(), { 0, 0 }, { 1, 1 });
	}

	void Scene::CollectRenderableEntities()
	{
		
	}
	std::vector<glm::mat4> Scene::GetModelSpaceBoneTransforms(const std::vector<UUID>& boneEntityIds, Ref<MeshSource> meshSource)
	{
		std::vector<glm::mat4> boneTransforms(boneEntityIds.size());
		if (meshSource)
		{
			if (const auto skeleton = meshSource->GetSkeleton(); skeleton)
			{
				// Can get mismatches if user changes which mesh an entity refers to after the bone entities have been set up
				// TODO(0x): need a better way to handle the bone entities
				//ASSERT(boneEntityIds.size() == skeleton.GetNumBones(), "Wrong number of boneEntityIds for mesh skeleton!");
				for (uint32_t i = 0; i < std::min(skeleton->GetNumBones(), (uint32_t)boneEntityIds.size()); ++i)
				{
					auto boneEntity = GetEntityByUUID(boneEntityIds[i]);
					glm::mat4 localTransform = boneEntity ? boneEntity.GetComponent<TransformComponent>().GetTransform() : glm::identity<glm::mat4>();
					auto parentIndex = skeleton->GetParentBoneIndex(i);
					boneTransforms[i] = (parentIndex == Skeleton::NullIndex) ? localTransform : boneTransforms[parentIndex] * localTransform;
				}
			}
		}
		return boneTransforms;
	}
	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
	{
		glm::mat4 transform(1.0f);

		Entity parent = GetEntityByUUID(entity.GetParentUUID());
		if (parent)
			transform = GetWorldSpaceTransformMatrix(parent);

		return transform * entity.Transform().GetTransform();
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

		SortEntities();

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
	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityIDMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}
	void Scene::ClearEntities()
	{
		m_Registry.each([this](entt::entity entityID) {
			Entity entity{ entityID, this };
			DestroyEntity(entity); // 执行自定义清理
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

	Entity Scene::BuildDynamicMeshEntity(Ref<MeshSource> mesh, Entity& root, const std::filesystem::path& path)
	{
		AssetHandle handle = mesh->Handle;
		root.AddComponent<AnimationComponent>(handle, path);
		auto com = root.GetComponent<AnimationComponent>();
		BuildMeshEntityHierarchy(root, mesh, mesh->GetRootNode());
		BuildBoneEntityIds(root);
		return root;
	}

	void Scene::BuildBoneEntityIds(Entity entity)
	{
		// 给这个Entity所有包含SubMesh组件的子Entity设置SubMesh组件的骨骼信息
		BuildMeshBoneEntityIds(entity, entity);

		//// AnimationComponent may not be a direct child of the entity.
		//// We must rebuild the animation component bone entity ids from the oldest ancestor
		// 从下往上找到谁包含动画组件
		Entity animationEntity = entity;
		Entity parent = entity.GetParent();
		while (parent)
		{
			if (parent.HasComponent<AnimationComponent>())
			{
				animationEntity = parent;
			}
			parent = parent.GetParent();
		}

		BuildAnimationBoneEntityIds(animationEntity, animationEntity);
	}
	void Scene::BuildAnimationBoneEntityIds(Entity entity, Entity rootEntity)
	{
		if (entity.HasComponent<AnimationComponent>()) {
			auto& anim = entity.GetComponent<AnimationComponent>();
			anim.BoneEntityIds = FindBoneEntityIds(entity, rootEntity, AssetManager::GetAsset<MeshSource>(anim.meshSource)->GetSkeleton());
		}
		for (auto childId : entity.Children())
		{
			Entity child = GetEntityByUUID(childId);
			if (child) {
				BuildAnimationBoneEntityIds(child, rootEntity);
			}
		}
	}
	void Scene::BuildMeshBoneEntityIds(Entity entity, Entity rootEntity)
	{
		if (entity.HasComponent<SubmeshComponent>()) {
			SubmeshComponent& mc = entity.GetComponent<SubmeshComponent>();
			AssetHandle meshSourceHandle = mc.Mesh;
			Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(meshSourceHandle);
			mc.BoneEntityIds = FindBoneEntityIds(entity, rootEntity, meshSource->GetSkeleton()); // 设置组件的骨骼信息
		}
		for (auto childId : entity.Children())
		{
			Entity child = GetEntityByUUID(childId);
			if (child) {
				BuildMeshBoneEntityIds(child, rootEntity);
			}
		}
	}
	std::vector<UUID> Scene::FindBoneEntityIds(Entity entity, Entity rootEntity, const Skeleton* skeleton)
	{
		std::vector<UUID> boneEntityIds;

		// 从下往上一层一层找，直到找到Skeleton所有的骨骼Entity
		if (skeleton)
		{
			Entity rootParentEntity = rootEntity ? rootEntity.GetParent() : rootEntity;
			{
				auto boneNames = skeleton->GetBoneNames();
				boneEntityIds.reserve(boneNames.size());
				bool foundAtLeastOne = false;
				for (const auto& boneName : boneNames)
				{
					bool found = false;
					Entity e = entity;
					while (e && e != rootParentEntity)
					{
						Entity boneEntity = TryGetDescendantEntityWithTag(e, boneName);
						if (boneEntity)
						{
							boneEntityIds.emplace_back(boneEntity.GetUUID());
							found = true;
							break;
						}
						e = e.GetParent();
					}
					if (found)
						foundAtLeastOne = true;
					else
						boneEntityIds.emplace_back(0);
				}
				if (!foundAtLeastOne)
					boneEntityIds.resize(0);
			}
		}
		return boneEntityIds;
	}
	Entity Scene::TryGetDescendantEntityWithTag(Entity entity, const std::string& tag)
	{
		//HZ_PROFILE_FUNC();
		if (entity)
		{
			if (entity.GetComponent<TagComponent>().Tag == tag)
				return entity;

			for (const auto childId : entity.Children())
			{
				Entity descendant = TryGetDescendantEntityWithTag(GetEntityByUUID(childId), tag);
				if (descendant)
					return descendant;
			}
		}
		return {};
	}
	void Scene::BuildMeshEntityHierarchy(Entity parent, Ref<MeshSource> meshSource, const MeshNode& node)
	{
		const auto& nodes = meshSource->GetNodes();

		// capture meshSource and nodes so we don't have to keep getting them as we recurse
		std::function<void(Entity, const MeshNode&)> recurse = [&](Entity parent, const MeshNode& node)
			{
				// Skip empty root node
				// We should still apply its transform though, as there will sometimes be a 90 degree rotation here
				// particularly for GLTF assets (where DCC tool may have tried to convert Z-up to Y-up)
				if (node.IsRoot() && node.Submeshes.size() == 0)
				{
					for (uint32_t child : node.Children)
					{
						MeshNode childNode = nodes[child];
						childNode.LocalTransform = node.LocalTransform * childNode.LocalTransform;
						recurse(parent, childNode);
					}
					return;
				}

				Entity nodeEntity = CreateChildEntity(parent, node.Name);
				nodeEntity.Transform().SetTransform(node.LocalTransform);
				//nodeEntity.AddComponent<MeshTagComponent>(); // TODO: (0x) Add correct root entity id

				if (node.Submeshes.size() == 1)
				{
					// Node == Mesh in this case
					uint32_t submeshIndex = node.Submeshes[0];
					auto& mc = nodeEntity.AddComponent<SubmeshComponent>(meshSource->Handle, submeshIndex);

					/*if (mesh->ShouldGenerateColliders())
					{
						auto& colliderComponent = nodeEntity.AddComponent<MeshColliderComponent>();
						Ref<MeshColliderAsset> colliderAsset = PhysicsSystem::GetOrCreateColliderAsset(nodeEntity, colliderComponent);
						colliderComponent.ColliderAsset = colliderAsset->Handle;
						colliderComponent.SubmeshIndex = submeshIndex;
						colliderComponent.UseSharedShape = colliderAsset->AlwaysShareShape;
						nodeEntity.AddComponent<RigidBodyComponent>();
					}*/
				}
				else if (node.Submeshes.size() > 1)
				{
					// Create one entity per child mesh, parented under node
					for (uint32_t i = 0; i < node.Submeshes.size(); i++)
					{
						uint32_t submeshIndex = node.Submeshes[i];

						// NOTE(Yan): original implemenation use to use "mesh name" from assimp;
						//            we don't store that so use node name instead. Maybe we
						//            should store it?
						Entity childEntity = CreateChildEntity(nodeEntity, node.Name);

						//childEntity.AddComponent<MeshTagComponent>(); // TODO: (0x) Add correct root entity id
						childEntity.AddComponent<SubmeshComponent>(meshSource->Handle, submeshIndex);

						/*if (mesh->ShouldGenerateColliders())
						{
							auto& colliderComponent = childEntity.AddComponent<MeshColliderComponent>();
							Ref<MeshColliderAsset> colliderAsset = PhysicsSystem::GetOrCreateColliderAsset(childEntity, colliderComponent);
							colliderComponent.ColliderAsset = colliderAsset->Handle;
							colliderComponent.SubmeshIndex = submeshIndex;
							colliderComponent.UseSharedShape = colliderAsset->AlwaysShareShape;
							childEntity.AddComponent<RigidBodyComponent>();
						}*/
					}
				}

				for (uint32_t child : node.Children)
					recurse(nodeEntity, nodes[child]);
			};

		recurse(parent, node);
	}
	Scene::~Scene()
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
	void Scene::OnComponentAdded<AnimationComponent>(Entity entity, AnimationComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<DynamicModelComponent>(Entity entity, DynamicModelComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<SubmeshComponent>(Entity entity, SubmeshComponent& component)
	{
	}
	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
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
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TextComponent>(Entity entity, TextComponent& component)
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




}
