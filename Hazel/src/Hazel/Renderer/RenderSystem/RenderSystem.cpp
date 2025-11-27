#include "hzpch.h"
#include "RenderSystem.h"
#include "Hazel/Core/Application.h"
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

namespace GameEngine {
	RenderSystem::RenderSystem()
	{
		m_DynamicRHI = DynamicRHI::Init({ API_Vulkan,true,false });
		m_Surface = m_DynamicRHI->CreateSurface(APP_GLFWWINDOW);
		m_GraphicsQueue = m_DynamicRHI->GetQueue({ QUEUE_TYPE_GRAPHICS, 0 });
		m_SwapChain = m_DynamicRHI->CreateSwapChain({ m_Surface, m_GraphicsQueue, FRAMES_IN_FLIGHT, m_Surface->GetExetent(), SWAPCHAIN_COLOR_FORMAT });
		m_CommandPool = m_DynamicRHI->CreateCommandPool({ m_GraphicsQueue });
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			m_PerFrameBaseResources[i].commandList = m_CommandPool->CreateCommandList(true);
			m_PerFrameBaseResources[i].startSemaphore = m_DynamicRHI->CreateSemaphore();
			m_PerFrameBaseResources[i].finishSemaphore = m_DynamicRHI->CreateSemaphore();
			m_PerFrameBaseResources[i].fence = m_DynamicRHI->CreateFence(true);
		}
	}

	void RenderSystem::Tick(float timestep)
	{
		LightCollector::CollectLight();
		MeshCollector::CollectMesh();
		m_RenderResourceManager->Tick();
		auto& CurResource = m_PerFrameBaseResources[APP_FRAMEINDEX];
		/// LOG_INFO("RenderSystem::Tick");
		CurResource.fence->Wait();
		RHITextureRef CurSwapchainTexture = m_SwapChain->GetNewFrame(nullptr, CurResource.startSemaphore);
		RHICommandListRef CurCommandList = CurResource.commandList;
		CurCommandList->BeginCommand();

		RDGBuilder rdgBuilder = RDGBuilder(CurCommandList);
		// 构建图结构
		for (auto& pass : passes) { if (pass) pass->Build(rdgBuilder); }
		// 执行RDG
		rdgBuilder.Execute();
		rdgDependencyGraph = rdgBuilder.GetGraph();
		CurCommandList->EndCommand();
		CurCommandList->Execute(CurResource.fence, CurResource.startSemaphore, CurResource.finishSemaphore);
		m_GPUTimeInfos = CurCommandList->GetGPUTime();
		m_SwapChain->Present(CurResource.finishSemaphore);
	}

	void RenderSystem::InitPasses()
	{

		m_RenderResourceManager = std::make_shared<RenderResourceManager>();

		passes[IBL_PASS] = std::make_shared<IBLPass>();
		meshPasses[MESH_PASS_DIRSHADOW_PASS] = std::make_shared<DirShadowPass>();
		meshPasses[MESH_PASS_GBUFFER_PASS] = std::make_shared<GBufferPass>();
		passes[DIR_SHADOW_PASS] = meshPasses[MESH_PASS_DIRSHADOW_PASS];
		passes[GBUFFER_PASS] = meshPasses[MESH_PASS_GBUFFER_PASS];
		passes[GRID_PASS] = std::make_shared<GridPass>();
		passes[SKY_PASS] = std::make_shared<SkyPass>();
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

}
