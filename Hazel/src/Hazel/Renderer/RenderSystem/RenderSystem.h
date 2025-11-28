#pragma once
#include "Hazel/Renderer/RHI/RHI.h"
#include "Hazel/Renderer/RHI/RHICommandList.h"
#include "Hazel/Core/Definations.h"
#include <Hazel/Renderer/RDG/DependencyGraph.h>
#include <Hazel/Renderer/RenderPass/RenderPass.h>
// #define RDG_DEBUG 
#define RENDER_RESOURCEMANAGER APP_RENDERSYSTEM->GetRenderResourceManager()
#define RENDER_GPU_TIME_INFO APP_RENDERSYSTEM->GetGPUTimeInfos()
namespace GameEngine
{
	class MeshPass;
	class RenderResourceManager;
	class RenderSystem
	{
	public:
		RenderSystem();
		void InitPasses();
		void Tick(float timestep);
		RHISwapchainRef GetSwapChain() { return m_SwapChain; }
		DynamicRHIRef GetRHI() { return m_DynamicRHI; }
		std::vector<RHIGPUTimeInfo>& GetGPUTimeInfos() { return m_GPUTimeInfos; }
		std::shared_ptr<RenderResourceManager> GetRenderResourceManager() { return m_RenderResourceManager; }
		DependencyGraphRef GetRDGDependenctyGraph() { return rdgDependencyGraph; }
		const std::array<std::shared_ptr<MeshPass>, MESH_PASS_TYPE_MAX_CNT>& GetMeshPasses() { return meshPasses; }

	private:
		// 处理器
		std::shared_ptr<RenderResourceManager> m_RenderResourceManager;
		// 渲染资源
		DynamicRHIRef m_DynamicRHI;
		RHISurfaceRef m_Surface;
		RHIQueueRef m_GraphicsQueue;
		RHISwapchainRef m_SwapChain;
		RHICommandPoolRef m_CommandPool;
		std::vector<RHIGPUTimeInfo> m_GPUTimeInfos;
		struct PerFrameBaseResource
		{
			RHICommandListRef commandList;
			RHISemaphoreRef startSemaphore;
			RHISemaphoreRef finishSemaphore;
			RHIFenceRef fence;
		};
		std::array<PerFrameBaseResource, FRAMES_IN_FLIGHT> m_PerFrameBaseResources;
		std::array<std::shared_ptr<RenderPassNew>, PASS_TYPE_MAX_CNT> passes;
		std::array<std::shared_ptr<MeshPass>, MESH_PASS_TYPE_MAX_CNT> meshPasses;

		DependencyGraphRef rdgDependencyGraph;

	};




}

