#pragma once
#include "Asset.h"
#include "Model.h"
#include "Hazel/Core/Definations.h"
#include <optional>
/*
     资产管理器功能：
	 1. 资产序列化和反序列化
	 2. 资产缓存和加载
	 3. 原始资源加载
*/
namespace GameEngine {
	class AssetManager {
	public:

		// 给定原始资源路径，序列化资产到 APP_SERIALIZE_PATH
		template<typename T>
		static void SerializeAsset(std::filesystem::path path) {
			if constexpr (std::is_same_v<T, Model>) {
				ModelRef loadedModel = LoadModel(path.string());
				std::filesystem::path savePath(APP_SERIALIZE_MODEL_PATH + path.stem().string() + "_" + loadedModel->GetUUID().ToString() + APP_SERIALIZE_MODEL_EXT);
				DoSerialize<Model>(loadedModel, savePath);
			}
		}

		// 反序列化资产
		template<typename T>
		static std::shared_ptr<T> DeserializeAsset(std::filesystem::path filePath) {
			if constexpr (std::is_same_v<T, Model>){
				if (AssetsMapByPath.find(filePath) != AssetsMapByPath.end()) {
					return  std::static_pointer_cast<T>(AssetsMapByPath[filePath]);
				}
				ModelRef loadedModel = std::make_shared<Model>();
				DoDeserialize<Model>(loadedModel, filePath);
				AssetsMapByPath[filePath.string()] = loadedModel;
				AssetsMapByUUID[loadedModel->GetUUID()] = loadedModel;
				return loadedModel;
			}
			else {
                LOG_ERROR("Asset not found and No Serialize file Find!");
                return nullptr;
			}
			 
		}

		// 获取资产，不存在时会寻找对应序列化文件并加载
		template<typename T>
		static std::shared_ptr<T> GetAsset(UUID handle) {
			std::shared_ptr<Asset> asset;
			if (AssetsMapByUUID.find(handle) == AssetsMapByUUID.end()) {
				if constexpr (std::is_same_v<T, Model>) {
					std::filesystem::path filePath = FindModelByUUID(APP_SERIALIZE_MODEL_PATH, handle).value();
					LOG_INFO("Model Not Find, Start Serialize from {}!", filePath.string());
					return DeserializeAsset<T>(filePath);
				}
				else {
					LOG_ERROR("Asset not found and No Serialize file Find!");
				}
			}
			return std::static_pointer_cast<T>(AssetsMapByUUID[handle]);
		}

		static ModelRef LoadModel(std::string path);

		static std::optional<std::filesystem::path> FindModelByUUID(const std::filesystem::path& basePath, const UUID& uuid)
		{
			if (!std::filesystem::exists(basePath))
				return std::nullopt;
			const std::string uuidStr = uuid.ToString();
			for (const auto& entry : std::filesystem::directory_iterator(basePath))
			{
				if (!entry.is_regular_file())
					continue;
				const auto& path = entry.path();
				if (path.extension() != ".hModel")
					continue;
				const std::string filename = path.stem().string();
				size_t pos = filename.rfind('_');
				if (pos == std::string::npos)
					continue;

				std::string fileUUID = filename.substr(pos + 1);

				if (fileUUID == uuidStr)
					return path;
			}

			return std::nullopt;
		}
	
	private:
		template<typename T>
		static void DoDeserialize(std::shared_ptr<T>& assetRef, std::filesystem::path& path) {
			std::ifstream is(path, std::ios::binary);
			if (!is.is_open()) { LOG_ERROR("Failed to open file for scene deserialization"); }
			cereal::BinaryInputArchive archive(is);
			archive(assetRef);
		}

		template<typename T>
		static void DoSerialize(std::shared_ptr<T>& assetRef, std::filesystem::path& path) {
			std::filesystem::create_directories(path.parent_path());
			std::ofstream os(path, std::ios::binary);
			if (!os.is_open()) { LOG_ERROR("Failed to open file for scene serialization"); }
			cereal::BinaryOutputArchive archive(os);
			archive(assetRef);
			LOG_INFO("Serialize to {0}", path);
		}

	private:

		static std::unordered_map<UUID, std::shared_ptr<Asset>> AssetsMapByUUID;
		static std::unordered_map<std::filesystem::path, std::shared_ptr<Asset>> AssetsMapByPath;
	};
}
