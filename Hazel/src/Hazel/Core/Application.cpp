#include "hzpch.h"

#include <nfd.hpp>
#include <GLFW/glfw3.h>
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Events/Event.h"
#include "Hazel/Core/Events/ApplicationEvent.h"
#include "Hazel/Scene/SceneManager.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"

namespace GameEngine {
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification)
		: m_Specification(specification)
	{
		Log::Init();
		ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		NFD::Init();

		m_WindowManager = std::make_shared<WindowManager>(WindowSpec(m_Specification.Name, 1950, 1300));
		m_WindowManager->SetEventCallback(HZ_BIND_EVENT_FN(Application::OnEvent));

		m_SceneManager = std::make_shared<SceneManager>();

		m_RenderSystem = std::make_shared<RenderManager>();
		m_RenderSystem->InitPasses();
	}

	void Application::Tick()
	{
		while (m_Running)
		{
			//m_RendererManager->ExecutePreFrame(); // RT_thread

			float timestep = GetTimePreFrame();

			m_SceneManager->Tick(timestep);

			//if (!m_Minimized) {
			//	m_RendererManager->Tick(timestep);
			//}

			//m_GLFWWindow->Tick();
			if (!m_Minimized) {
				m_RenderSystem->Tick(timestep);
			}
			m_WindowManager->Tick();
			m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % FRAMES_IN_FLIGHT;
			tick++;
		}
	}
	float Application::GetTimePreFrame()
	{
		float time = glfwGetTime();
		Timestep timestep = time - m_LastFrameTime;
		m_LastFrameTime = time;
		return timestep;
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(HZ_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(Application::OnWindowResize));
		dispatcher.Dispatch<WindowMinimizeEvent>(HZ_BIND_EVENT_FN(Application::OnWindowMinimize));
		if (m_RenderSystem->OnEvent(e)) {
			return;
		}
	}
	bool Application::OnWindowMinimize(WindowMinimizeEvent& e)
	{
		m_Minimized = e.IsMinimized();

		return false;
	}
	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		const uint32_t width = e.GetWidth(), height = e.GetHeight();
		if (width == 0 || height == 0)
		{
			//m_Minimized = true;
			return false;
		}
		//m_Minimized = false;

		//auto& window = m_GLFWWindow;
		//RENDER_SUBMIT([&window, width, height]() mutable
		//	{
		//		window->GetSwapChain().OnResize(width, height);
		//	});

		return false;
	}

	std::shared_ptr<GameEngine::RendererManager> Application::GetRendererManager()
	{
		return Get().m_RendererManager;
	}

	std::shared_ptr<GameEngine::RenderManager> Application::GetRenderSystem()
	{
		return Get().m_RenderSystem;
	}
}