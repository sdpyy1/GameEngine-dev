#pragma once
#include "Hazel/Core/Timestep.h"
#include "Scene.h"
#include "Hazel/Scene/EditorCamera.h"
#include "Hazel/Renderer/RenderResource/RenderStruct.h"

namespace GameEngine
{
	// 只在CPU传递的设置数据
	struct CPURenderSetting {
		std::string IBLPath;
	};
	struct SceneInfo
	{
		GlobalSettingInfo globalSettingInfos;
		CPURenderSetting cpuRenderSetting;
		std::shared_ptr<EditorCamera> camera;
		std::shared_ptr<EditorCamera> defaultCamera;
	};
	class SceneManager
	{
	public:
		SceneManager();
		void Tick(Timestep ts);

		const std::shared_ptr<EditorCamera> GetActiveEditorCamera() { return m_ActiveEditorCamera; };
		const std::shared_ptr<EditorCamera> GetDefaultEditorCamera() { return m_DefaultEditorCamera; };
		std::pair<unsigned int, unsigned int> GetViewportSize();
		std::shared_ptr<Scene> GetActiveScene() { return m_CurrentScene; };
		void SetActiveScene(std::shared_ptr<Scene> scene) { m_CurrentScene = scene; m_CurrentSceneFilePath = ""; };
		SceneInfo GetSceneInfo(){return m_SceneInfo;}
		bool OpenScene();
		bool OpenScene(const std::filesystem::path& filepath);
		void SaveScene();
		void SaveSceneAs();
		void SetDebugImageName(std::string name) {DebugImageName = name;};
        std::string GetDebugImageName() { return DebugImageName; };
	public:
		bool HasDirLight();
	private:
		void PackInfo();
		void PackSettingForRender();

		SceneInfo m_SceneInfo;
		std::shared_ptr<Scene> m_CurrentScene;
		std::shared_ptr<EditorCamera> m_DefaultEditorCamera;
		std::shared_ptr<EditorCamera> m_ActiveEditorCamera;
		std::string m_CurrentSceneFilePath;
		std::string DebugImageName;
	};

}
