#include "hzpch.h"
#include "AssetManager.h"
namespace GameEngine {

	std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::AssetsMap;
	std::unordered_map<std::string, std::shared_ptr<Model>> AssetManager::ModelCache;

}
