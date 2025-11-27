#include "hzpch.h"
#include "SceneManager.h"
#include "Hazel/Utils/FileSystem.h"
#include "SceneSerializer.h"
#include "Hazel/Core/Application.h"
namespace GameEngine
{
	SceneManager::SceneManager()
	{
		m_CurrentScene = std::make_shared<Scene>();
		
		m_EditorCamera = std::make_shared<EditorCamera>(45.0f, APP_WINDOWSIZE.first, APP_WINDOWSIZE.second, 0.1f, 1000.0f);
	}

	void SceneManager::Tick(Timestep ts)
	{
		// LOG_TRACE("SceneManager::Tick");
		m_EditorCamera->OnUpdate(ts);
		m_CurrentScene->PackupSceneInfo(*m_EditorCamera); // 打包场景数据
		m_CurrentScene->UpdateAnimation(ts); // 更新动画
	}

	bool SceneManager::OpenScene()
	{
		std::filesystem::path filepath = FileSystem::OpenFileDialog({ { "GameEngine Scene", "hscene" } });
		if (!filepath.empty())
			return OpenScene(filepath);

		return false;
	}
	bool SceneManager::OpenScene(const std::filesystem::path& filepath)
	{
		if (filepath.extension() != ".hscene")
		{
			return false;
		}
		if (!FileSystem::Exists(filepath))
		{
			LOG_ERROR("Tried loading a non-existing scene: {0}", filepath);
			return false;
		}

		if (m_CurrentScene) {
			m_CurrentScene->ClearEntities();
		}
		SceneSerializer serializer(m_CurrentScene);  // 目前打开一个场景，就是把当前场景清空，加载新场景的Entity
		serializer.Deserialize(filepath.string());
		m_CurrentSceneFilePath = filepath.string();
		std::replace(m_CurrentSceneFilePath.begin(), m_CurrentSceneFilePath.end(), '\\', '/');
		return true;
	}

	void SceneManager::SaveScene()
	{
		if (!m_CurrentSceneFilePath.empty())
		{
			SceneSerializer serializer(m_CurrentScene);
			serializer.Serialize(m_CurrentSceneFilePath);
		}
		else
		{
			SaveSceneAs();
		}
	}

	void SceneManager::SaveSceneAs()
	{
		std::filesystem::path filepath = FileSystem::SaveFileDialog({ { "GameEngine Scene (*.hscene)", "hscene" } });

		if (filepath.empty())
			return;

		if (!filepath.has_extension())
			filepath += SceneSerializer::DefaultExtension;

		SceneSerializer serializer(m_CurrentScene);
		serializer.Serialize(filepath.string());

		std::filesystem::path path = filepath;
		m_CurrentSceneFilePath = filepath.string();
		std::replace(m_CurrentSceneFilePath.begin(), m_CurrentSceneFilePath.end(), '\\', '/');
	}

	bool SceneManager::HasDirLight()
	{
		return m_CurrentScene->HasDirLight();
	}

}
