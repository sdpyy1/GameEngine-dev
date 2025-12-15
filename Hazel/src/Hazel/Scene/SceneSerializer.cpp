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
		std::ofstream os(filepath, std::ios::binary);
		if (!os.is_open()){LOG_ERROR("Failed to open file for scene serialization");}
		cereal::BinaryOutputArchive archive(os);
		archive(cereal::make_nvp("Scene", *m_Scene));
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream is(filepath, std::ios::binary);
		if (!is.is_open())
		{
			LOG_ERROR("Failed to open file for scene deserialization");
		}
		cereal::BinaryInputArchive archive(is);
		archive(cereal::make_nvp("Scene", *m_Scene));
		return true;
	}
}
