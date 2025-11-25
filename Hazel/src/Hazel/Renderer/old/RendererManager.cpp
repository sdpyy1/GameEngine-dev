#include "hzpch.h"
#include "RendererManager.h"
#include "Renderer.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/SceneRender.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Core/Application.h"
namespace GameEngine {
	RendererManager::RendererManager() :m_RenderThread(ThreadingPolicy::SingleThreaded)
	{
		m_RenderThread.Run();

		Renderer::Init();
		m_RenderThread.Pump();


		m_ImGuiRendererManager = ImGuiRendererManager::Create();
		m_SceneRender = std::make_shared<SceneRender>();
		m_RenderThread.Pump();
	}


	void RendererManager::Tick(float timestep)
	{
		// LOG_TRACE("RenderManager::Tick {}",Renderer::GetCurrentFrameIndex());
		Application::GetSceneManager()->GetActiveScene()->CollectRenderableEntities();

		Renderer::BeginFrame();

		m_SceneRender->PreRender(Application::GetSceneManager()->GetActiveScene()->GetSceneInfo());
		m_SceneRender->EndRender();

		RenderImGui(timestep);

		Renderer::EndFrame();
		// LOG_TRACE("RenderManager::Tick Done!");
	}


	void RendererManager::RenderImGui(float timestep)
	{
		RENDER_SUBMIT([this, timestep]()
			{
				m_ImGuiRendererManager->Tick(timestep);
			});
	}

	void RendererManager::ExecutePreFrame()
	{
		m_RenderThread.BlockUntilRenderComplete();
		// xxxx
		m_RenderThread.NextFrame();
		m_RenderThread.Kick();
	}

	bool RendererManager::OnEvent(Event& e)
	{
		bool isHandle = false;
		isHandle = m_ImGuiRendererManager->OnEvent(e);
		return isHandle;
	}

	Ref<GameEngine::Image2D> RendererManager::GetTextureWhichNeedDebug() const
	{
		return m_SceneRender->GetTextureWhichNeedDebug();
	}

	Ref<GameEngine::Image2D> RendererManager::GetFinalImage() const
	{
		return m_SceneRender->GetFinalImage();
	}

}
