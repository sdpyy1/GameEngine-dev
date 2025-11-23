#include "hzpch.h"
#include "RenderSystem.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderPass/GridPass.h"
#include <Hazel/Renderer/RenderPass/ImGuiPass.h>
#include <Hazel/Renderer/RenderPass/PresentPass.h>
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

		CurCommandList->EndCommand();
		CurCommandList->Execute(CurResource.fence, CurResource.startSemaphore, CurResource.finishSemaphore);
		m_SwapChain->Present(CurResource.finishSemaphore);
	}

	void RenderSystem::InitPasses()
	{
		m_RenderResourceManager = std::make_shared<RenderResourceManager>();

		passes[GRID_PASS] = std::make_shared<GridPass>();
		passes[IMGUI_PASS] = std::make_shared<ImGuiPass>();
        passes[PRESENT_PASS] = std::make_shared<PresentPass>();


		for (auto& pass : passes) {
			if (pass) {
				pass->Init();
			}
		}
	}

}
