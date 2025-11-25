#include "hzpch.h"
#include "Volk/volk.h"
#include "AssetManager.h"
namespace GameEngine {
	std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::m_MemoryAssets;
	std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>>AssetManager::m_AssetDependencies;
	std::map<std::filesystem::path, Ref<Asset>>AssetManager::MesheSourceCacheMap;
	std::unordered_map<AssetHandle, std::shared_ptr<V2::Asset>> AssetManager::AssetsMap;

}
