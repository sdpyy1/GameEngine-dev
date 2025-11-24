#pragma once
namespace GameEngine {
#define FRAMES_IN_FLIGHT 3							//Ö¡»º³åÊýÄ¿

#define APP_FRAMEINDEX Application::GetFrameIndex()
#define APP_GLFWWINDOW Application::GetWindowManager()->GetGLFWWindow()
#define APP_RENDERSYSTEM Application::GetRenderSystem()
#define APP_SWAPCHAIN Application::GetRenderSystem()->GetSwapChain()
#define APP_DYNAMICRHI Application::GetRenderSystem()->GetRHI()
#define APP_WINDOWMINIMIZED Application::Get().isMinimized()
#define APP_WINDOWSIZE Application::Get().GetWindowManager()->GetWindowSize()
#define APP_SCENEMANAGER Application::GetSceneManager()
#define APP_TICK Application::GetTotalTick()
#define SWAPCHAIN_COLOR_FORMAT FORMAT_R8G8B8A8_UNORM
}