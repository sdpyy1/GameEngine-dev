#pragma once
#include "Asset.h"
#include "Model.h"
namespace GameEngine {
	class AssetManager {
	public:
		static ModelRef LoadModel(std::string path);


		template<typename T>
		static std::shared_ptr<T> GetAssetByAssetHandle(UUID assetHandle) {
			auto asset = AssetsMap[assetHandle];
			if (!asset) return nullptr;
			return std::static_pointer_cast<T>(asset);
		}

	private:
		static std::unordered_map<UUID, std::shared_ptr<Asset>> AssetsMap;
		static std::unordered_map<std::string, std::shared_ptr<Model>> ModelCache;
	};
}
