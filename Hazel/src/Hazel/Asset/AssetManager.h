#pragma once
#include "Asset.h"
#include "Model.h"
#include "Hazel/Core/Definations.h"
#include <optional>
#include <functional>
#include <filesystem>
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

			// 用源文件路径派生稳定 UUID：同一源模型每次序列化的 UUID 一致，
			// 文件名固定 -> 重复序列化直接覆盖旧文件，而不是生成新的随机 UUID 文件，
			// 同时保证场景中对模型 UUID 的引用不会因重新序列化而失效。
			uint64_t stableUID = std::hash<std::string>()(std::filesystem::absolute(path).string());
			loadedModel->SetUUID(UUID(stableUID));

			// 序列化前删除同一源模型之前的序列化文件（按文件名前缀匹配），
			// 避免历史遗留的随机 UUID 文件越积越多。
			if (std::filesystem::exists(APP_SERIALIZE_MODEL_PATH)) {
				const std::string stem = path.stem().string();
				std::error_code ec;
				for (const auto& entry : std::filesystem::directory_iterator(APP_SERIALIZE_MODEL_PATH)) {
					if (!entry.is_regular_file()) continue;
					const auto& ep = entry.path();
					if (ep.extension() != ".hModel") continue;
					std::string name = ep.stem().string();
					size_t pos = name.rfind('_');
					if (pos != std::string::npos && name.substr(0, pos) == stem) {
						std::filesystem::remove(ep, ec);
					}
				}
			}
				std::filesystem::path savePath(APP_SERIALIZE_MODEL_PATH + path.stem().string() + "_" + loadedModel->GetUUID().ToString() + APP_SERIALIZE_MODEL_EXT);
				DoSerialize<Model>(loadedModel, savePath);
			}
		}

		// 给定已序列化的模型文件，从中记录的源路径重新序列化并覆盖。
		// 适用于源模型被修改（如切线/法线修复）后，不用重新定位源文件即可一键重导。
		static void ReserializeAsset(std::filesystem::path serializedModelPath) {
			if (serializedModelPath.extension().string() != APP_SERIALIZE_MODEL_EXT) {
				LOG_ERROR("ReserializeAsset: [{}] is not a serialized model.", serializedModelPath.string());
				return;
			}
			// 反序列化已序列化模型，读取其内部记录的源文件路径（Model 序列化时保存的是原始资源路径）
			ModelRef loadedModel = DeserializeAsset<Model>(serializedModelPath);
			if (!loadedModel) {
				LOG_ERROR("ReserializeAsset: failed to deserialize [{}].", serializedModelPath.string());
				return;
			}
			std::string sourcePath = loadedModel->GetPath();
			// 仅为了读取源路径而反序列化，立即从缓存移除并释放其 GPU 资源，避免泄漏
			AssetsMapByPath.erase(serializedModelPath.string());
			AssetsMapByUUID.erase(loadedModel->GetUUID());
			loadedModel = nullptr;

			if (sourcePath.empty() || !std::filesystem::exists(sourcePath)) {
				LOG_ERROR("ReserializeAsset: source path [{}] not found, cannot reserialize [{}].", sourcePath, serializedModelPath.string());
				return;
			}
			LOG_INFO("ReserializeAsset: reserializing [{}] from source [{}]", serializedModelPath.string(), sourcePath);
			SerializeAsset<Model>(sourcePath);
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
