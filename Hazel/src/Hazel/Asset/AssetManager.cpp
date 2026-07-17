#include "hzpch.h"
#include "AssetManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
namespace GameEngine {

	std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::AssetsMapByUUID;
	std::unordered_map<std::filesystem::path, std::shared_ptr<Asset>> AssetManager::AssetsMapByPath;



	// 只是为了序列化时先加载原始模型数据，不需要CPU信息
	ModelRef AssetManager::LoadModel(std::string path) {
		ModelSpec m_ModelSpec;
		return std::make_shared<Model>(path, m_ModelSpec);
	}

}
