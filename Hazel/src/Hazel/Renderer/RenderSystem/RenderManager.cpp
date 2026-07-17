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
#include <Hazel/Renderer/RenderPass/PathTracingPass.h>
#include <Hazel/Renderer/RenderPass/TAAPass.h>
#include <Hazel/Renderer/RenderPass/DDGIPass.h>
#include <Hazel/Renderer/RenderPass/ExposurePass.h>
#include <Hazel/Renderer/RenderPass/GPUCullingPass.h>
#include <Hazel/Renderer/RenderPass/ClusterLightingPass.h>
#include <Hazel/Renderer/RenderPass/SVGFPass.h>
#include <Hazel/Renderer/RenderPass/FXAAPass.h>
#include <Hazel/Renderer/RenderPass/SSSRPass.h>

namespace GameEngine {
	RenderManager::RenderManager()
	{
		RHIConfig config{ API_Vulkan,true,true, };
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

	void RenderManager::Tick(float timestep)
	{
		LightCollector::CollectLight();
		MeshCollector::CollectMesh();
		m_RenderResourceManager->Tick();
		auto& CurResource = m_PerFrameBaseResources[APP_FRAMEINDEX];
		CurResource.fence->WaitAndReset();

		RHITextureRef CurSwapchainTexture = m_SwapChain->GetNewFrame(nullptr, CurResource.startSemaphore);
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
		CurCommandList->Execute(CurResource.fence, CurResource.startSemaphore, CurResource.finishSemaphore);

		m_GPUTimeInfos = CurCommandList->GetGPUTime();
		m_SwapChain->Present(CurResource.finishSemaphore);
		m_DynamicRHI->Tick();

	}

	void RenderManager::InitPasses()
	{
		m_RenderResourceManager = std::make_shared<RenderResourceManager>();

		passes[IBL_PASS] = std::make_shared<IBLPass>();
		passes[GPUCULLING_PASS] = std::make_shared<GPUCullingPass>();
		passes[CLUSTER_LIGHTING_PASS] = std::make_shared<ClusterLightingPass>();
		meshPasses[MESH_PASS_DIRSHADOW_PASS] = std::make_shared<DirShadowPass>();
		meshPasses[MESH_PASS_POINTSHADOW_PASS] = std::make_shared<PointShadowPass>();
		meshPasses[MESH_PASS_GBUFFER_PASS] = std::make_shared<GBufferPass>();
		meshPasses[MESH_PASS_PREDEPTH_PASS] = std::make_shared<PreDepthPass>();
		passes[DIR_SHADOW_PASS] = meshPasses[MESH_PASS_DIRSHADOW_PASS];
		passes[POINT_SHADOW_PASS] = meshPasses[MESH_PASS_POINTSHADOW_PASS];
		passes[GBUFFER_PASS] = meshPasses[MESH_PASS_GBUFFER_PASS];
		if (RENDER_ENABLE_RAY_TRACING) {
			passes[PATHTRACING_PASS] = std::make_shared<PathTracingPass>();
			// passes[SVGF_PASS] = std::make_shared<SVGFPass>();  SVGF还没写明白
			passes[DDGI_PASS] = std::make_shared<DDGIPass>();
		}
		passes[PREDEPTH_PASS] = meshPasses[MESH_PASS_PREDEPTH_PASS];
		passes[GRID_PASS] = std::make_shared<GridPass>();
		passes[GIZMO_PASS] = std::make_shared<GizmoPass>();
		passes[SKY_PASS] = std::make_shared<SkyPass>();
		passes[LIGHT_PASS] = std::make_shared<LightPass>();
		passes[TAA_PASS] = std::make_shared<TAAPass>();
		passes[FXAA_PASS] = std::make_shared<FXAAPass>();
		passes[BLOOM_PASS] = std::make_shared<BloomPass>();
		passes[EXPOSURE_PASS] = std::make_shared<ExposurePass>();
		passes[POST_PROCESS_PASS] = std::make_shared<PostProcessPass>();
		passes[IMGUI_PASS] = std::make_shared<ImGuiPass>();
		passes[PRESENT_PASS] = std::make_shared<PresentPass>();
		passes[SSSR_PASS] = std::make_shared<SSSRPass>();

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
}