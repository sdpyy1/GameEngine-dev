#include "hzpch.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"
#include "Hazel/Core/UUID.h"

#include <fstream>
#include "Hazel/Asset/AssetManager.h"
#include <yaml-cpp/yaml.h>
#include "Hazel/Asset/Model/Mesh.h"
namespace YAML {
	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<GameEngine::UUID>
	{
		static Node encode(const GameEngine::UUID& uuid)
		{
			Node node;
			node.push_back((uint64_t)uuid);
			return node;
		}

		static bool decode(const Node& node, GameEngine::UUID& uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};
}

namespace GameEngine {
#define WRITE_SCRIPT_FIELD(FieldType, Type)           \
			case ScriptFieldType::FieldType:          \
				out << scriptField.GetValue<Type>();  \
				break

#define READ_SCRIPT_FIELD(FieldType, Type)             \
	case ScriptFieldType::FieldType:                   \
	{                                                  \
		Type data = scriptField["Data"].as<Type>();    \
		fieldInstance.SetValue(data);                  \
		break;                                         \
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	static std::string RigidBody2DBodyTypeToString(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody2DComponent::BodyType::Static:    return "Static";
		case Rigidbody2DComponent::BodyType::Dynamic:   return "Dynamic";
		case Rigidbody2DComponent::BodyType::Kinematic: return "Kinematic";
		}

		ASSERT(false, "Unknown body type");
		return {};
	}

	static Rigidbody2DComponent::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static")    return Rigidbody2DComponent::BodyType::Static;
		if (bodyTypeString == "Dynamic")   return Rigidbody2DComponent::BodyType::Dynamic;
		if (bodyTypeString == "Kinematic") return Rigidbody2DComponent::BodyType::Kinematic;

		ASSERT(false, "Unknown body type");
		return Rigidbody2DComponent::BodyType::Static;
	}

	SceneSerializer::SceneSerializer(const std::shared_ptr<Scene> scene)
		: m_Scene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity, std::shared_ptr<Scene> scene)
	{
		ASSERT(entity.HasComponent<IDComponent>());

		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}
		if (entity.HasComponent<RelationshipComponent>())
		{
			auto& relationshipComponent = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Value << relationshipComponent.ParentHandle;

			out << YAML::Key << "Children";
			out << YAML::Value << YAML::BeginSeq;

			for (auto child : relationshipComponent.Children)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << child;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}
		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent
			auto& transform = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.GetRotationEuler();
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;
			out << YAML::EndMap; // TransformComponent
		}
		if (entity.HasComponent<ModelComponent>())
		{
			out << YAML::Key << "ModelComponent";
			out << YAML::BeginMap; // ModelComponent

			auto& smc = entity.GetComponent<ModelComponent>();
			out << YAML::Key << "MeshSourcePath" << YAML::Value << smc.path.string();
			out << YAML::Key << "Visible" << YAML::Value << smc.Visible;
			out << YAML::EndMap; // ModelComponent
		}
		if (entity.HasComponent<DynamicModelComponent>())
		{
			out << YAML::Key << "DynamicModelComponent";
			out << YAML::BeginMap; // ModelComponent

			auto& smc = entity.GetComponent<DynamicModelComponent>();
			out << YAML::Key << "MeshSourcePath" << YAML::Value << smc.path.string();
			out << YAML::EndMap; // ModelComponent
		}
		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap; // DirectionalLightComponent

			auto& directionalLightComponent = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Intensity" << YAML::Value << directionalLightComponent.Intensity;
			out << YAML::Key << "Radiance" << YAML::Value << directionalLightComponent.Radiance;
			out << YAML::Key << "CastShadows" << YAML::Value << directionalLightComponent.CastShadows;
			out << YAML::Key << "SoftShadows" << YAML::Value << directionalLightComponent.SoftShadows;
			out << YAML::Key << "LightSize" << YAML::Value << directionalLightComponent.LightSize;
			out << YAML::Key << "ShadowAmount" << YAML::Value << directionalLightComponent.ShadowAmount;

			out << YAML::EndMap; // DirectionalLightComponent
		}
        if (entity.HasComponent<SpotLightComponent>())
		{ 
			out << YAML::Key << "SpotLightComponent";
			out << YAML::BeginMap;
            auto& spotLightComponent = entity.GetComponent<SpotLightComponent>();
            out << YAML::Key << "Direction" << YAML::Value << spotLightComponent.Direction;
            out << YAML::Key << "Intensity" << YAML::Value << spotLightComponent.Intensity;
            out << YAML::Key << "Radiance" << YAML::Value << spotLightComponent.Radiance;
            out << YAML::Key << "Position" << YAML::Value << spotLightComponent.Position;
            out << YAML::EndMap;
		}
        if (entity.HasComponent<SkyComponent>())
		{ 
            out << YAML::Key << "SkyComponent";
            out << YAML::BeginMap;
            auto& skyComponent = entity.GetComponent<SkyComponent>();
            out << YAML::Key << "DynamicSky" << YAML::Value << skyComponent.DynamicSky;
            out << YAML::Key << "selectedIBL" << YAML::Value << skyComponent.selectedIBL;
			out << YAML::EndMap;
		}
		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		// Sort entities by UUID (for better serializing)
		std::map<UUID, entt::entity> sortedEntityMap;
		auto idComponentView = m_Scene->m_Registry.view<IDComponent>();
		for (auto entity : idComponentView)
		{
			sortedEntityMap[idComponentView.get<IDComponent>(entity).ID] = entity;
		}

		// Serialize sorted entities
		for (auto [id, entity] : sortedEntityMap)
		{
			SerializeEntity(out, { entity, m_Scene.get() }, m_Scene);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			LOG_ERROR("Failed to load .hazel file '{0}'\n     {1}", filepath, e.what());
			return false;
		}

		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		LOG_TRACE("Deserializing scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				LOG_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntity(name);
				auto& relationshipComponent = deserializedEntity.GetComponent<RelationshipComponent>();
				uint64_t parentHandle = entity["Parent"] ? entity["Parent"].as<uint64_t>() : 0;
				relationshipComponent.ParentHandle = parentHandle;

				auto children = entity["Children"];
				if (children)
				{
					for (auto child : children)
					{
						uint64_t childHandle = child["Handle"].as<uint64_t>();
						relationshipComponent.Children.push_back(childHandle);
					}
				}
				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.SetRotation(transformComponent["Rotation"].as<glm::vec3>());
					tc.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				auto staticMeshComponent = entity["ModelComponent"];
				if (staticMeshComponent) {
					deserializedEntity.AddComponent<ModelComponent>();
					Ref<Asset> meshSource = AssetManager::GetMesh(staticMeshComponent["MeshSourcePath"].as<std::string>());
					auto& com = deserializedEntity.GetComponent<ModelComponent>();
					com.path = staticMeshComponent["MeshSourcePath"].as<std::string>();
					com.StaticMesh = meshSource->Handle;
					com.Visible = staticMeshComponent["Visible"].as<bool>();
				}
				auto dynamicMeshComponent = entity["DynamicModelComponent"];
				if (dynamicMeshComponent) {
					Ref<Asset> meshSource = AssetManager::GetMesh(dynamicMeshComponent["MeshSourcePath"].as<std::string>());
					deserializedEntity.AddComponent<DynamicModelComponent>(meshSource->Handle, dynamicMeshComponent["MeshSourcePath"].as<std::string>());
					m_Scene->BuildDynamicMeshEntity(meshSource, deserializedEntity, dynamicMeshComponent["MeshSourcePath"].as<std::string>());
				}
				if (auto directionalLightComponent = entity["DirectionalLightComponent"]; directionalLightComponent)
				{
					auto& component = deserializedEntity.AddComponent<DirectionalLightComponent>();
					component.Intensity = directionalLightComponent["Intensity"].as<float>(1.0f);
					component.Radiance = directionalLightComponent["Radiance"].as<glm::vec3>(glm::vec3(1.0f));
					component.CastShadows = directionalLightComponent["CastShadows"].as<bool>(true);
					component.SoftShadows = directionalLightComponent["SoftShadows"].as<bool>(true);
					component.LightSize = directionalLightComponent["LightSize"].as<float>(0.5f);
					component.ShadowAmount = directionalLightComponent["ShadowAmount"].as<float>(1.0f);
				}
				if (auto spotLightComponent = entity["SpotLightComponent"]; spotLightComponent)
				{
                    deserializedEntity.AddComponent<SpotLightComponent>();
                    deserializedEntity.GetComponent<SpotLightComponent>().Direction = spotLightComponent["Direction"].as<glm::vec3>(glm::vec3(0.0f, -1.0f, 0.0f));
                    deserializedEntity.GetComponent<SpotLightComponent>().Intensity = spotLightComponent["Intensity"].as<float>(1.0f);
                    deserializedEntity.GetComponent<SpotLightComponent>().Radiance = spotLightComponent["Radiance"].as<glm::vec3>(glm::vec3(1.0f));
					deserializedEntity.GetComponent<SpotLightComponent>().Position = spotLightComponent["Position"].as<glm::vec3>(glm::vec3(0.0f));
				}
				if (auto skyComponent = entity["SkyComponent"]; skyComponent)
				{
                    deserializedEntity.AddComponent<SkyComponent>();
                    deserializedEntity.GetComponent<SkyComponent>().DynamicSky = skyComponent["DynamicSky"].as<bool>(false);
                    deserializedEntity.GetComponent<SkyComponent>().selectedIBL = skyComponent["selectedIBL"].as<uint32_t>(0);
				}
			}
		}

		return true;
	}
}
