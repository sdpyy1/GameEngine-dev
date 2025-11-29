#pragma once
#include <Hazel/Core/Events/Event.h>
#include <GLFW/glfw3.h>
namespace GameEngine { 
	struct WindowSpec
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool VSync;
		WindowSpec(const std::string& title = "GameEngine Engine",
			uint32_t width = 800,
			uint32_t height = 600, bool VSync = false)
			: Title(title), Width(width), Height(height), VSync(VSync)
		{
		}
	};
	class WindowManager
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;
		WindowManager(const WindowSpec& props);
		void SetEventCallback(const EventCallbackFn& callback);
		GLFWwindow* GetGLFWWindow() { return m_GLFWWindow; }
		std::pair<unsigned int, unsigned int> GetWindowSize() const{return { m_Data.Width, m_Data.Height };}
		void Tick();
        ~WindowManager();
	private:
		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};
		GLFWwindow* m_GLFWWindow;
		WindowData m_Data;

	};





}

