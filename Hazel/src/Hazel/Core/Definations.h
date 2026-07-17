#pragma once
namespace GameEngine {
#define FRAMES_IN_FLIGHT 3							//帧缓冲数目

#define APP_FRAMEINDEX Application::GetFrameIndex()
#define APP_GLFWWINDOW Application::GetWindowManager()->GetGLFWWindow()
#define APP_RENDERSYSTEM Application::GetRenderSystem()
#define APP_SWAPCHAIN Application::GetRenderSystem()->GetSwapChain()
#define APP_DYNAMICRHI Application::GetRenderSystem()->GetRHI()
#define APP_WINDOWMINIMIZED Application::Get().isMinimized()
#define APP_WINDOWSIZE Application::Get().GetWindowManager()->GetWindowSize()
#define APP_SCENEMANAGER Application::GetSceneManager()
#define APP_TICK Application::GetTotalTick()
#define APP_SCENE_CAMERA Application::GetSceneManager()->GetActiveEditorCamera()
#define APP_SCENE_DEFAULT_CAMERA Application::GetSceneManager()->GetDefaultEditorCamera()



#define APP_WORKING_DIR std::string("D:/AAA_GameEngine_Dev/Hazel/") // TODO: 写死了属于是 应该去Application去拿

#define APP_SHADER_PATH std::string(APP_WORKING_DIR + "Assets/Shader/")
#define APP_HDR_PATH std::string(APP_WORKING_DIR + "Assets/HDR/")
#define APP_TEXTURE_PATH std::string(APP_WORKING_DIR + "Assets/Texture/")
#define APP_ICON_PATH std::string(APP_WORKING_DIR + "Assets/Icon/")
#define APP_MODEL_PATH std::string(APP_WORKING_DIR + "Assets/Model/")
#define APP_ASSET_PATH std::string(APP_WORKING_DIR + "Assets/")
#define APP_SERIALIZE_PATH std::string(APP_WORKING_DIR + "Assets/Serialize/")
#define APP_SERIALIZE_MODEL_PATH std::string(APP_SERIALIZE_PATH + "model/")
#define APP_SERIALIZE_MATERIAL_PATH std::string(APP_SERIALIZE_PATH + "material/")
#define APP_SERIALIZE_SCENE_PATH std::string(APP_SERIALIZE_PATH + "scene/")




#define APP_SERIALIZE_MODEL_EXT std::string(".hModel")
}