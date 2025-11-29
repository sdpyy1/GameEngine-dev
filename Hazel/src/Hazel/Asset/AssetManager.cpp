#include "hzpch.h"
#include "Volk/volk.h"
#include "AssetManager.h"
namespace GameEngine {

	std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::AssetsMap;
}
