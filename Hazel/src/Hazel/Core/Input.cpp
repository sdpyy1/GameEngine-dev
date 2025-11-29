#include "hzpch.h"
#include "Hazel/Core/Input.h"

#include "Hazel/Core/Application.h"

#include <GLFW/glfw3.h>
#include <imgui_internal.h>

namespace GameEngine {
	bool Input::IsKeyDown(KeyCode keycode)
	{
		GLFWwindow* win = static_cast<GLFWwindow*>(APP_GLFWWINDOW);
		ImGuiContext* context = ImGui::GetCurrentContext();
		bool pressed = false;
		for (ImGuiViewport* viewport : context->Viewports)
		{
			if (!viewport->PlatformUserData)
				continue;

			GLFWwindow* windowHandle = *(GLFWwindow**)viewport->PlatformUserData; // First member is GLFWwindow
			if (!windowHandle)
				continue;
			auto state = glfwGetKey(windowHandle, static_cast<int32_t>(keycode));
			if (state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				pressed = true;
				break;
			}
		}

		return pressed;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		ImGuiContext* context = ImGui::GetCurrentContext();
		bool pressed = false;
		for (ImGuiViewport* viewport : context->Viewports)
		{
			if (!viewport->PlatformUserData)
				continue;

			GLFWwindow* windowHandle = *(GLFWwindow**)viewport->PlatformUserData; // First member is GLFWwindow
			if (!windowHandle)
				continue;

			auto state = glfwGetMouseButton(static_cast<GLFWwindow*>(windowHandle), static_cast<int32_t>(button));
			if (state == GLFW_PRESS || state == GLFW_REPEAT)
			{
				pressed = true;
				break;
			}
		}
		return pressed;
	}

	

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		return (float)x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return (float)y;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		double x, y;
		glfwGetCursorPos(APP_GLFWWINDOW, &x, &y);
		return { (float)x, (float)y };
	}

	// TODO: A better way to do this is to handle it internally, and simply move the cursor the opposite side
	//		of the screen when it reaches the edge
	void Input::SetCursorMode(CursorMode mode)
	{
		glfwSetInputMode(APP_GLFWWINDOW, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
	}

	CursorMode Input::GetCursorMode()
	{
		return (CursorMode)(glfwGetInputMode(APP_GLFWWINDOW, GLFW_CURSOR) - GLFW_CURSOR_NORMAL);
	}
}
