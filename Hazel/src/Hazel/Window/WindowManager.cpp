#include "hzpch.h"
#include "WindowManager.h"
#include <Hazel/Core/Events/ApplicationEvent.h>
#include <Hazel/Core/Events/KeyEvent.h>
#include <Hazel/Core/Events/MouseEvent.h>
namespace GameEngine {
	static void GLFWErrorCallback(int error, const char* description)
	{
		LOG_ERROR("GLFW error ({0}): {1}", error, description);
	}

	WindowManager::WindowManager(const WindowSpec& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;
		m_Data.VSync = props.VSync;
		LOG_INFO("Creating window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);
		int success = glfwInit();
		ASSERT(success, "Could not initialize GLFW!");
		glfwSetErrorCallback(GLFWErrorCallback);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_GLFWWindow = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(m_GLFWWindow, &m_Data);		
		
		glfwSetWindowUserPointer(m_GLFWWindow, &m_Data);
		// SetVSync(&m_Data.VSync);  Sync的设置在RenderSystem中

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_GLFWWindow, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_GLFWWindow, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_GLFWWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event((KeyCode)key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event((KeyCode)key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event((KeyCode)key, true);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(m_GLFWWindow, [](GLFWwindow* window, unsigned int keycode)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				KeyTypedEvent event((KeyCode)keycode);
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(m_GLFWWindow, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event((MouseButton)button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event((MouseButton)button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_GLFWWindow, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_GLFWWindow, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});
		glfwSetWindowIconifyCallback(m_GLFWWindow, [](GLFWwindow* window, int iconified)
			{
				auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
				WindowMinimizeEvent event((bool)iconified);
				data.EventCallback(event);
			});

		// Update window size to actual size
		{
			int width, height;
			glfwGetWindowSize(m_GLFWWindow, &width, &height);
			m_Data.Width = width;
			m_Data.Height = height;
		}
	}

	WindowManager::~WindowManager()
	{
		glfwDestroyWindow(m_GLFWWindow);
		glfwTerminate();
	}

	void WindowManager::Tick()
	{
		glfwPollEvents();
	}

	void WindowManager::SetEventCallback(const EventCallbackFn& callback)
	{
		m_Data.EventCallback = callback;
	}

}

