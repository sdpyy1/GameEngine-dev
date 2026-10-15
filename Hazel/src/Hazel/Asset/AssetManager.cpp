#include "hzpch.h"
#include "AssetManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include <system_error>
namespace GameEngine {

	std::unordered_map<UUID, std::shared_ptr<Asset>> AssetManager::AssetsMapByUUID;
	std::unordered_map<std::filesystem::path, std::shared_ptr<Asset>> AssetManager::AssetsMapByPath;

	ModelMemoryStats AssetManager::InspectModel(const std::filesystem::path& sourcePath)
	{
		ModelMemoryStats stats;
		stats.inspected = true;
		std::error_code ec;
		stats.sourceBytes = std::filesystem::file_size(sourcePath, ec);

		ModelSpec spec;
		spec.loadMaterials = false;
		spec.uploadGPU = false;
		spec.genBLAS = false;
		spec.buildDistanceField = false;

		try {
			Model model(sourcePath.string(), spec);
			if (!model.IsValid()) {
				stats.error = "Assimp did not produce any submeshes. The format or required decoder may be unsupported.";
				return stats;
			}
			stats.modelObjectBytes = sizeof(Model);
			for (const SubmeshData& submeshData : model.GetSubmeshes()) {
				if (!submeshData.mesh)
					continue;

				const Mesh& mesh = *submeshData.mesh;
				MeshMemoryStats meshStats;
				meshStats.name = mesh.name;
				meshStats.vertexCount = mesh.position.size();
				meshStats.indexCount = mesh.index.size();
				meshStats.triangleCount = mesh.index.size() / 3;
				meshStats.positionBytes = mesh.position.capacity() * sizeof(glm::vec3);
				meshStats.normalBytes = mesh.normal.capacity() * sizeof(glm::vec3);
				meshStats.tangentBytes = mesh.tangent.capacity() * sizeof(glm::vec4);
				meshStats.texCoordBytes = mesh.texCoord.capacity() * sizeof(glm::vec2);
				meshStats.colorBytes = mesh.color.capacity() * sizeof(glm::vec3);
				meshStats.boneIndexBytes = mesh.boneIndex.capacity() * sizeof(glm::ivec4);
				meshStats.boneWeightBytes = mesh.boneWeight.capacity() * sizeof(glm::vec4);
				meshStats.indexBytes = mesh.index.capacity() * sizeof(uint32_t);
				meshStats.boneBytes = mesh.bone.capacity() * sizeof(BoneInfo);
				for (const BoneInfo& bone : mesh.bone)
					meshStats.boneBytes += bone.name.capacity() * sizeof(char);
				meshStats.objectBytes = sizeof(Mesh);

				stats.vertexCount += meshStats.vertexCount;
				stats.indexCount += meshStats.indexCount;
				stats.triangleCount += meshStats.triangleCount;
				stats.totalBytes += meshStats.TotalBytes();
				stats.submeshes.push_back(std::move(meshStats));
			}
			stats.totalBytes += stats.modelObjectBytes;
			stats.valid = true;
		}
		catch (const std::exception& exception) {
			stats.error = exception.what();
		}
		return stats;
	}



	// 只是为了序列化时先加载原始模型数据，不需要CPU信息
	ModelRef AssetManager::LoadModel(std::string path) {
		ModelSpec m_ModelSpec;
		return std::make_shared<Model>(path, m_ModelSpec);
	}

}
