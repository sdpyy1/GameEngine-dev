#include "hzpch.h"
#include "AssetManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
namespace GameEngine {

	std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::AssetsMap;
	std::unordered_map<std::string, std::shared_ptr<Model>> AssetManager::ModelCache;

	ModelRef AssetManager::LoadModel(std::string path) {
		if (ModelCache.find(path) != ModelCache.end())
			return ModelCache[path];
		ModelSpec m_ModelSpec;

		if (RENDER_ENABLE_RAY_TRACING) {
			m_ModelSpec.genBLAS = true;
		}

		auto& model = std::make_shared<Model>(path, m_ModelSpec);
		AssetsMap[model->GetUUID()] = model;
		ModelCache[path] = model;
		return model;
	}

}
