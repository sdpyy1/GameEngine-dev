#include "hzpch.h"
#include "MeshCollector.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Scene/Components.h>
#include <Hazel/Scene/Entity.h>
#include "Hazel/Renderer/RenderPass/RenderPass.h"
namespace GameEngine
{
	void GameEngine::MeshCollector::CollectMesh()
	{
		std::vector<DrawBatch> batch;

		auto& scene = APP_SCENEMANAGER->GetActiveScene();
		// 收集SkeletalMesh
		auto allEntityOwnSubmesh = scene->GetAllEntitiesWith<SubmeshComponent>();
		for (auto entity : allEntityOwnSubmesh)
		{
			auto meshComponent = allEntityOwnSubmesh.get<SubmeshComponent>(entity);
			Entity parent = Entity(entity, scene);
			if (meshComponent.model == nullptr || !parent.GetParent().GetComponent<ModelComponent>().Visible ||!meshComponent.Visible) continue;

			Entity e = Entity(entity, scene.get());
			glm::mat4 transform = scene->GetWorldSpaceTransformMatrix(e);
			meshComponent.meshInfo.modelMatrix = transform;
			meshComponent.updateMeshInfo();  // 需要更新

			DrawBatch drawBatch;
			drawBatch.indexBuffer = meshComponent.model->GetSubmesh(meshComponent.SubmeshIndex).indexBuffer;
            drawBatch.vertexBuffer = meshComponent.model->GetSubmesh(meshComponent.SubmeshIndex).vertexBuffer;
			drawBatch.objectID = meshComponent.meshInfoID;
            drawBatch.material = meshComponent.material;
			batch.push_back(drawBatch);
		}


		for (auto& meshPass : APP_RENDERSYSTEM->GetMeshPasses()) {

			if (!meshPass) continue;

			for (auto& processor : meshPass->GetMeshPassProcessors())
			{
				processor->Process(batch);
			}
		}
	}
}

