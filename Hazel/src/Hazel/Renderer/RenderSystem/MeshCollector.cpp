#include "hzpch.h"
#include "MeshCollector.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/SceneManager.h"
#include <Hazel/Scene/Components.h>
#include <Hazel/Scene/Entity.h>
#include "Hazel/Renderer/RenderPass/RenderPass.h"
namespace GameEngine
{
	/*
		这里的收集类似UE的 FScene-> Proxy(并没有做) -> FMeshBatch -> 创建MeshPassProcessor -> 每个Pass定义自己AddBatch()收集自己需要的MeshBatch -> 最终组织成FMeshDrawCommand
	*/
	void GameEngine::MeshCollector::CollectMesh()
	{
		std::vector<MeshBatch> batch;

		auto& scene = APP_SCENEMANAGER->GetActiveScene();
		// 这个遍历在UE相当于从FPrimitiveSceneProxy到FMeshBatch
		auto& allEntityOwnSubmesh = scene->GetAllEntitiesWith<SubmeshComponent>();
		for (auto& entity : allEntityOwnSubmesh)
		{
			auto& meshComponent = allEntityOwnSubmesh.get<SubmeshComponent>(entity);
			Entity meshEntity = Entity(entity, scene);
			if (meshComponent.model == nullptr || !meshEntity.GetParent().GetComponent<ModelComponent>().Visible || !meshComponent.Visible) continue;

			glm::mat4 transform = scene->GetWorldSpaceTransformMatrix(meshEntity); // 因为SubMesh存的都是Local变换

			//////////////////////////////////////////////// TODO: CPU剔除 ////////////////////////////////////////////
			// 可以做一个Model级剔除，GPU再进行mesh级剔除  这里对Model的剔除可以捎带学习了各种加速结构

			//////////////////////////////////////////////// 收集MeshBatch ////////////////////////////////////////////
			MeshBatch drawBatch;
			drawBatch.instanceID = meshComponent.meshInfoID;
			drawBatch.material = meshComponent.GetMaterial();
			drawBatch.indexCount = meshComponent.model->GetSubmeshes()[meshComponent.SubmeshIndex].indexBuffer->IndexNum();
			batch.push_back(drawBatch);

			//////////////////////////////////////////////// 更新实例信息 ////////////////////////////////////////////
			meshComponent.meshInfo.modelMatrix = transform;
			meshComponent.meshInfo.prevModelMatrix = meshComponent.prevModel;
			if (meshComponent.meshInfoID == 0) {
				meshComponent.meshInfoID = RENDER_RESOURCEMANAGER->AllocateMeshInstanceInfoID(); // 实例信息还没传递到GPU
			}
			meshComponent.meshInfo.animationID = 0; // TODO: 动画
			meshComponent.meshInfo.indexID = meshComponent.model->GetSubmeshes()[meshComponent.SubmeshIndex].indexBuffer->indexID;
			meshComponent.meshInfo.vertexID = meshComponent.model->GetSubmeshes()[meshComponent.SubmeshIndex].vertexBuffer->vertexID;
			meshComponent.meshInfo.materialID = meshComponent.material? meshComponent.material->GetMaterialID():0;

			RENDER_RESOURCEMANAGER->SetMeshInstanceInfo(meshComponent.meshInfo, meshComponent.meshInfoID);  //TODO:目前是一个Mesh一个Mesh上传数据到GPU，需要合并上传，但是涉及到如何合并的问题
			meshComponent.prevModel = transform;
		}

		for (auto& meshPass : APP_RENDERSYSTEM->GetMeshPasses()) {
			if (!meshPass) continue;
			for(auto& processor : meshPass->GetMeshPassProcessors()){
				processor->Process(batch);
			}
		}

		if (RENDER_ENABLE_RAY_TRACING && !batch.empty()) {
			std::vector<RHIAccelerationStructureInstanceInfo> instances;
			Collect4TLAS(instances);
			RENDER_RESOURCEMANAGER->UpdateTLAS(instances);
		}
	}

	/*
		收集所有Mesh的BLAS和ModelMatrix，用于更新TLAS
	*/
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