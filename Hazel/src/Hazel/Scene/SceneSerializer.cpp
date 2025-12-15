#include "hzpch.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"
#include "Hazel/Core/UUID.h"

#include <fstream>
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Utils/Serializable.h"

namespace GameEngine {

	SceneSerializer::SceneSerializer(const std::shared_ptr<Scene> scene)
		: m_Scene(scene)
	{
	}


	void SceneSerializer::Serialize(const std::string& filepath)
	{
		std::ofstream os(filepath);
		if (!os.is_open()){LOG_ERROR("Failed to open file for scene serialization");}
		cereal::JSONOutputArchive archive(os);
		archive(cereal::make_nvp("Scene", *m_Scene));
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream is(filepath);
		if (!is.is_open())
		{
			LOG_ERROR("Failed to open file for scene deserialization");
		}
		cereal::JSONInputArchive archive(is);
		archive(cereal::make_nvp("Scene", *m_Scene));
		return true;
	}
}
