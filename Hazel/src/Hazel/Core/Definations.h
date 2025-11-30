#pragma once
namespace GameEngine {
#define FRAMES_IN_FLIGHT 2							//Ö¡»º³åÊýÄ¿

#define APP_FRAMEINDEX Application::GetFrameIndex()
#define APP_GLFWWINDOW Application::GetWindowManager()->GetGLFWWindow()
#define APP_RENDERSYSTEM Application::GetRenderSystem()
#define APP_SWAPCHAIN Application::GetRenderSystem()->GetSwapChain()
#define APP_DYNAMICRHI Application::GetRenderSystem()->GetRHI()
#define APP_WINDOWMINIMIZED Application::Get().isMinimized()
#define APP_WINDOWSIZE Application::Get().GetWindowManager()->GetWindowSize()
#define APP_SCENEMANAGER Application::GetSceneManager()
#define APP_TICK Application::GetTotalTick()
#define APP_SCENE_CAMERA Application::GetSceneManager()->GetEditorCamera()

#define APP_SHADER_PATH std::string("D:/AAA_GameEngine/Hazel/Assets/Shader/spv/")
#define APP_HDR_PATH std::string("D:/AAA_GameEngine/Hazel/Assets/HDR/")
#define APP_TEXTURE_PATH std::string("D:/AAA_GameEngine/Hazel/Assets/Texture/")
#define APP_ICON_PATH std::string("D:/AAA_GameEngine/Hazel/Assets/Icon/")
#define APP_MODEL_PATH std::string("D:/AAA_GameEngine/Hazel/Assets/Model/")



}