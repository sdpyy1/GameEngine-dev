#pragma once
#include "Hazel/Renderer/RHI/RHI.h"
#include "Hazel/Renderer/RHI/RHICommandList.h"
#include "Hazel/Core/Definations.h"
#include <Hazel/Renderer/RDG/DependencyGraph.h>
#include <Hazel/Renderer/RenderPass/RenderPass.h>
#include "Hazel/Core/Events/Event.h"
#include <Hazel/Core/RenderThread.h>
#include "RenderCommandQueue.h"
// #define RDG_DEBUG
#define RENDER_RESOURCEMANAGER APP_RENDERSYSTEM->GetRenderResourceManager()
#define RENDER_GPU_TIME_INFO APP_RENDERSYSTEM->GetGPUTimeInfos()
#define RENDER_ENABLE_RAY_TRACING APP_RENDERSYSTEM->IsEnableRayTracing()
namespace GameEngine
{
	class MeshPass;
	class RenderResourceManager;
	class PanelManager;
	class RenderManager
	{
	public:
		RenderManager();
		void RenderPrevFrame();
		void InitPasses();
		void Tick(float timestep);
		RHISwapchainRef GetSwapChain() { return m_SwapChain; }
		DynamicRHIRef GetRHI() { return m_DynamicRHI; }
		std::vector<RHIGPUTimeInfo>& GetGPUTimeInfos() { return m_GPUTimeInfos; }
		std::shared_ptr<RenderResourceManager> GetRenderResourceManager() { return m_RenderResourceManager; }
		void SetPanelManager(std::shared_ptr<PanelManager> panelManager);
		DependencyGraphRef GetRDGDependenctyGraph() { return rdgDependencyGraph; }
		const std::array<std::shared_ptr<MeshPass>, MESH_PASS_TYPE_MAX_CNT>& GetMeshPasses() { return meshPasses; }
		bool OnEvent(Event& e);
		uint32_t GetDrawCallCount() { return m_DrawCallCount; }
		bool IsEnableRayTracing() { return m_RHIConfig.enableRayTracing; }
		void SetDrawMeshCount(uint32_t count) { m_DrawMeshCount = count; }
		uint32_t GetDrawMeshCount() { return m_DrawMeshCount; }
		void SwapRenderCommandQueue();

		// RT
		static RenderCommandQueue& GetRenderCommandQueue();
		template<typename FuncT>
		static void Submit(FuncT&& func, const char* file = nullptr, int line = 0, const char* function = nullptr)
		{
			auto& queue = GetRenderCommandQueue();
#ifdef RTDEBUG
			queue.m_DebugInfos.push_back({ file, line, function });
#endif
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();
				pFunc->~FuncT();
				};

			auto storageBuffer = queue.Allocate(renderCmd, sizeof(FuncT));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}
		static uint32_t GetRenderQueueIndex();
		static void WaitAndRender(RenderThread* renderThread);

		static void RenderThreadFunc(RenderThread* renderThread);



	private:
		// 处理器
		std::shared_ptr<RenderResourceManager> m_RenderResourceManager;
		std::shared_ptr<PanelManager> m_PanelManager;
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
		uint32_t m_DrawCallCount = 0;
		uint32_t m_DrawMeshCount = 0;
		RHIConfig m_RHIConfig;

		RenderThread m_RenderThread;
	};
}
