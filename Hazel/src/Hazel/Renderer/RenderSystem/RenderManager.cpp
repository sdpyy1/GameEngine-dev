#include "hzpch.h"
#include "RenderManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Editor/PanelManager.h"
#include "Hazel/Renderer/RenderPass/GridPass.h"
#include <Hazel/Renderer/RenderPass/ImGuiPass.h>
#include <Hazel/Renderer/RenderPass/PresentPass.h>
#include "Hazel/Asset/Model.h"
#include <Hazel/Renderer/RenderPass/GbufferPass.h>
#include "MeshCollector.h"
#include "LightCollector.h"
#include <Hazel/Renderer/RenderResource/RenderResourceManager.h>
#include <Hazel/Renderer/RenderPass/IBLPass.h>
#include <Hazel/Renderer/RenderPass/DirShadowPass.h>
#include <Hazel/Renderer/RenderPass/SkyPass.h>
#include <Hazel/Renderer/RenderPass/BloomPass.h>
#include <Hazel/Renderer/RenderPass/PostProcessPass.h>
#include <Hazel/Renderer/RenderPass/LightPass.h>
#include <Hazel/Renderer/RenderPass/PreDepthPass.h>
#include <Hazel/Renderer/RenderPass/GizmoPass.h>
#include <Hazel/Renderer/RenderPass/PointShadowPass.h>
#include <Hazel/Renderer/RenderPass/RayTracingPass.h>
#include <Hazel/Renderer/RenderPass/TAAPass.h>
#include <Hazel/Renderer/RenderPass/DDGIPass.h>

namespace GameEngine {
	constexpr static uint32_t s_RenderCommandQueueCount = 2;
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;
	RenderManager::RenderManager():m_RenderThread(ThreadingPolicy::MultiThreaded) // SingleThreaded  MultiThreaded
	{
		m_RenderThread.Run();
		for (int i = 0; i < s_RenderCommandQueueCount; i++) {
			s_CommandQueue[i] = new RenderCommandQueue();
		}
		RHIConfig config;
		config.debug = true;
		config.enableRayTracing = true;
		config.api = API_Vulkan;
		m_RHIConfig = config;
		m_DynamicRHI = DynamicRHI::Init(config);
		m_Surface = m_DynamicRHI->CreateSurface(APP_GLFWWINDOW);
		m_GraphicsQueue = m_DynamicRHI->GetQueue({ QUEUE_TYPE_GRAPHICS, 0 });
		m_SwapChain = m_DynamicRHI->CreateSwapChain({ m_Surface, m_GraphicsQueue, FRAMES_IN_FLIGHT, m_Surface->GetExetent(), SWAPCHAIN_COLOR_FORMAT });
		m_CommandPool = m_DynamicRHI->CreateCommandPool({ m_GraphicsQueue });
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			m_PerFrameBaseResources[i].commandList = m_CommandPool->CreateCommandList(false);
			m_PerFrameBaseResources[i].startSemaphore = m_DynamicRHI->CreateSemaphore();
			m_PerFrameBaseResources[i].finishSemaphore = m_DynamicRHI->CreateSemaphore();
			m_PerFrameBaseResources[i].fence = m_DynamicRHI->CreateFence(true);
		}
	}

	void RenderManager::RenderPrevFrame() {

		m_RenderThread.BlockUntilRenderComplete(); // 等待RenderCommandQueue执行结束

		SwapRenderCommandQueue(); // 交换RenderCommandQueue（交换后的queue用于收集）

		m_RenderThread.Kick(); // 让RT开始工作

	}

	void RenderManager::Tick(float timestep)
	{
		RenderPrevFrame();
		LightCollector::CollectLight();
		MeshCollector::CollectMesh();
		//LOG_INFO("收集第{}帧", APP_FRAMEINDEX);
		m_RenderResourceManager->Tick();
		auto& CurResource = m_PerFrameBaseResources[APP_FRAMEINDEX];
		RHICommandListRef CurCommandList = CurResource.commandList;

		CurCommandList->BeginCommand();
		CurCommandList->ClearDrawCallCount();
		RDGBuilder rdgBuilder = RDGBuilder(CurCommandList);
		// 构建图结构
		for (auto& pass : passes) { if (pass) pass->Build(rdgBuilder); }
		// 执行RDG
		rdgBuilder.Execute();
		rdgDependencyGraph = rdgBuilder.GetGraph();
		m_DrawCallCount = CurCommandList->GetDrawCallCount();
		CurCommandList->EndCommand();
		RENDER_SUBMIT([this]() {
			//LOG_INFO("渲染第{}帧", APP_FRAMEINDEX_RT);
			auto& CurResource = m_PerFrameBaseResources[APP_FRAMEINDEX_RT];
			CurResource.fence->Wait(); // 先等待这个飞行帧上一帧渲染结束
			m_SwapChain->GetNewFrame(nullptr, CurResource.startSemaphore);
			RHICommandListRef CurCommandList = CurResource.commandList;
			CurCommandList->Execute(CurResource.fence, CurResource.startSemaphore, CurResource.finishSemaphore);
			m_GPUTimeInfos = CurCommandList->GetGPUTime();
			m_SwapChain->Present(CurResource.finishSemaphore);
			m_RenderThread.NextFrame();

		});
	}

	void RenderManager::InitPasses()
	{
		m_RenderResourceManager = std::make_shared<RenderResourceManager>();

		passes[IBL_PASS] = std::make_shared<IBLPass>();
		meshPasses[MESH_PASS_DIRSHADOW_PASS] = std::make_shared<DirShadowPass>();
		meshPasses[MESH_PASS_POINTSHADOW_PASS] = std::make_shared<PointShadowPass>();
		meshPasses[MESH_PASS_GBUFFER_PASS] = std::make_shared<GBufferPass>();
		meshPasses[MESH_PASS_PREDEPTH_PASS] = std::make_shared<PreDepthPass>();
		passes[DIR_SHADOW_PASS] = meshPasses[MESH_PASS_DIRSHADOW_PASS];
		passes[POINT_SHADOW_PASS] = meshPasses[MESH_PASS_POINTSHADOW_PASS];
		passes[GBUFFER_PASS] = meshPasses[MESH_PASS_GBUFFER_PASS];
		if (RENDER_ENABLE_RAY_TRACING) {
			passes[RAYTRACING_PASS] = std::make_shared<RayTracingPass>();
			passes[DDGI_PASS] = std::make_shared<DDGIPass>();
		}
		passes[PREDEPTH_PASS] = meshPasses[MESH_PASS_PREDEPTH_PASS];
		passes[GRID_PASS] = std::make_shared<GridPass>();
		passes[GIZMO_PASS] = std::make_shared<GizmoPass>();
		passes[SKY_PASS] = std::make_shared<SkyPass>();
		passes[LIGHT_PASS] = std::make_shared<LightPass>();
		passes[TAA_PASS] = std::make_shared<TAAPass>();
		passes[BLOOM_PASS] = std::make_shared<BloomPass>();
		passes[POST_PROCESS_PASS] = std::make_shared<PostProcessPass>();
		passes[IMGUI_PASS] = std::make_shared<ImGuiPass>();
		passes[PRESENT_PASS] = std::make_shared<PresentPass>();

		for (auto& pass : passes) {
			if (pass) {
				pass->Init();
			}
		}
	}

	void RenderManager::SetPanelManager(std::shared_ptr<PanelManager> panelManager)
	{
		m_PanelManager = panelManager;
	}

	bool RenderManager::OnEvent(Event& e)
	{
		if (m_PanelManager) {
			m_PanelManager->OnEvent(e);
		}
		return false;
	}
	void RenderManager::SwapRenderCommandQueue()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}
	RenderCommandQueue& RenderManager::GetRenderCommandQueue()
	{
		return *s_CommandQueue[s_RenderCommandQueueSubmissionIndex];
	}


	// 挂在渲染线程的函数
	void RenderManager::RenderThreadFunc(RenderThread* renderThread)
	{
		while (renderThread->IsRunning())
		{
			WaitAndRender(renderThread);
		}
	}

	uint32_t RenderManager::GetRenderQueueIndex()
	{
		return (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}
	void RenderManager::WaitAndRender(RenderThread* renderThread)
	{
		// Wait for kick, then set render thread to busy
		{
			// 渲染线程循环等待Kick信号，收到信号后，设置为Busy信号并开始工作
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);
		}
		// 工作就是把缓存命令全部执行
		s_CommandQueue[GetRenderQueueIndex()]->Execute();

		// Rendering has completed, set state to idle
		renderThread->Set(RenderThread::State::Idle);
	}
}