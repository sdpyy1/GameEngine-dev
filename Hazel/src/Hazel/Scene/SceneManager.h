#pragma once
#include "Hazel/Core/Timestep.h"
#include "Scene.h"
#include "Hazel/Scene/EditorCamera.h"
namespace GameEngine
{
	class SceneManager
	{
	public:
		SceneManager();
		void Tick(Timestep ts);

		std::shared_ptr<EditorCamera> GetEditorCamera() { return m_EditorCamera; };
		std::pair<unsigned int, unsigned int> GetViewportSize();;
		std::shared_ptr<Scene> GetActiveScene() { return m_CurrentScene; };
		SceneInfo GetSceneInfo(){return m_CurrentScene->GetSceneInfo();}
		bool OpenScene();
		bool OpenScene(const std::filesystem::path& filepath);
		void SaveScene();
		void SaveSceneAs();

	public:
		bool HasDirLight();
	private:
		std::shared_ptr<Scene> m_CurrentScene;
		std::shared_ptr<EditorCamera> m_EditorCamera;
		std::string m_CurrentSceneFilePath;

	};

}
