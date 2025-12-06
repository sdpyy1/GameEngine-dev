#pragma once
#include "Asset.h"
#include "Model.h"
namespace GameEngine {
	class AssetManager {
	public:
		static ModelRef LoadModel(std::string path,UUID uuid = 0);

		
		template<typename T>
		static std::shared_ptr<T> GetAsset(UUID handle) {
			auto asset = AssetsMap[handle];
			if (!asset) { 

				// TODO： 需要持久化UUID与资产加载信息的映射关系，在这里执行加载，而不是在外部判断null手动加载
				if constexpr (std::is_same_v<T, Model>) {
					
				}
				return nullptr;
			
			
			}
			return std::static_pointer_cast<T>(asset);
		}

	private:
		static std::unordered_map<UUID, std::shared_ptr<Asset>> AssetsMap;
		static std::unordered_map<std::string, std::shared_ptr<Model>> ModelCache;
	};
}
