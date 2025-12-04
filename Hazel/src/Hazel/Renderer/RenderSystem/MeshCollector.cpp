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
		auto& allEntityOwnSubmesh = scene->GetAllEntitiesWith<SubmeshComponent>();
		for (auto entity : allEntityOwnSubmesh)
		{
			auto &meshComponent = allEntityOwnSubmesh.get<SubmeshComponent>(entity);
			Entity parent = Entity(entity, scene);
			if (meshComponent.model == nullptr || !parent.GetParent().GetComponent<ModelComponent>().Visible ||!meshComponent.Visible) continue;

			Entity e = Entity(entity, scene.get());
			glm::mat4 transform = scene->GetWorldSpaceTransformMatrix(e);
			meshComponent.meshInfo.modelMatrix = transform;
			meshComponent.meshInfo.prevModelMatrix = meshComponent.prevModel;
			meshComponent.updateMeshInfo();  //TODO: 改成批量更新....
			meshComponent.prevModel = transform;
			DrawBatch drawBatch;
			drawBatch.indexBuffer = meshComponent.model->GetSubmeshData(meshComponent.SubmeshIndex).indexBuffer;
            drawBatch.vertexBuffer = meshComponent.model->GetSubmeshData(meshComponent.SubmeshIndex).vertexBuffer;
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


		if (RENDER_ENABLE_RAY_TRACING && !batch.empty()) {
			std::vector<RHIAccelerationStructureInstanceInfo> instances;
			Collect4TLAS(instances);
			RENDER_RESOURCEMANAGER->UpdateTLAS(instances);
		}


	}
	void MeshCollector::Collect4TLAS(std::vector<RHIAccelerationStructureInstanceInfo>& instances)
	{
		RHIBottomLevelAccelerationStructureRef blas;  // TODO: 其实和光栅无关的结构不应该放这
		auto& scene = APP_SCENEMANAGER->GetActiveScene();
		auto allEntityOwnSubmesh = scene->GetAllEntitiesWith<SubmeshComponent>();
		for (auto entity : allEntityOwnSubmesh)
		{
			auto meshComponent = allEntityOwnSubmesh.get<SubmeshComponent>(entity);
			Entity parent = Entity(entity, scene);
			if (meshComponent.model == nullptr || !parent.GetParent().GetComponent<ModelComponent>().Visible || !meshComponent.Visible) continue;

			Entity e = Entity(entity, scene.get());
			glm::mat4 transform = scene->GetWorldSpaceTransformMatrix(e);
			RHIAccelerationStructureInstanceInfo info = {};
			info.instanceIndex = meshComponent.meshInfoID;
			info.mask = 0xFF;
			info.shaderBindingTableOffset = 0;
			info.blas = meshComponent.model->GetSubmeshData(meshComponent.SubmeshIndex).blas;
			Math::ConvertGlmMat4To3x4Transform(transform, &info.transform[0][0]);
			instances.push_back(info);
			
		}


	}

}

