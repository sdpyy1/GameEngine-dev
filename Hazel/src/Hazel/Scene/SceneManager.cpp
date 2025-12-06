#include "hzpch.h"
#include "SceneManager.h"
#include "Hazel/Utils/FileSystem.h"
#include "SceneSerializer.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Math/Halton.h"
namespace GameEngine
{
	SceneManager::SceneManager()
	{
		m_CurrentScene = std::make_shared<Scene>();

		m_EditorCamera = std::make_shared<EditorCamera>(45.0f, Application::Get().GetWindowManager()->GetWindowSize().first, Application::Get().GetWindowManager()->GetWindowSize().second, 0.1f, 1000.0f);
	}

	void SceneManager::PackSettingForRender() {
		// 灯光设置
		auto dirLight = m_CurrentScene->GetFirstEntityWith<DirectionalLightComponent>();
		Entity dirLightEntity = Entity{ dirLight ,m_CurrentScene };
		if (dirLightEntity) {
			auto& component = dirLightEntity.GetComponent<DirectionalLightComponent>();
			m_SceneInfo.globalSettingInfos.shadowSetting.ShadowType = component.shadowType;
			m_SceneInfo.globalSettingInfos.shadowSetting.DebugCSM = component.showCSM;
		}

		// 后处理设置
		auto postprocess = m_CurrentScene->GetFirstEntityWith<PostProcessingComponent>();
		Entity postProcessEntity = Entity{ postprocess ,m_CurrentScene };
		if (postProcessEntity) {
			auto& component = postProcessEntity.GetComponent<PostProcessingComponent>();
			m_SceneInfo.globalSettingInfos.postprocess.bloomScale = component.bloomScale;
			m_SceneInfo.globalSettingInfos.postprocess.TaaSetting.enable = component.enableTAA;
			m_SceneInfo.globalSettingInfos.postprocess.TaaSetting.shaper = component.taaSharpen;
			m_SceneInfo.globalSettingInfos.postprocess.TaaSetting.shaperStrength = component.taaSharpness;
			m_SceneInfo.globalSettingInfos.postprocess.TaaSetting.UVjetter = Halton::GetTAAJetter(APP_TICK);
		}
		// 天空设置
		auto skyLight = m_CurrentScene->GetFirstEntityWith<SkyComponent>();
		Entity skyLightEntity = Entity{ skyLight ,m_CurrentScene };
		if (skyLightEntity) {
			auto& component = skyLightEntity.GetComponent<SkyComponent>();
			m_SceneInfo.globalSettingInfos.skySetting.isDynamicSky = component.DynamicSky ? 1 : 0;
			m_SceneInfo.globalSettingInfos.skySetting.IbLScale = component.IBLScale;
			m_SceneInfo.cpuRenderSetting.IBLPath = component.iblPath[component.selectedIBL].string();
		}
	}

	void SceneManager::PackInfo() {
		m_SceneInfo = {};
		m_SceneInfo.camera = m_EditorCamera;
		PackSettingForRender();
	};

	void SceneManager::Tick(Timestep ts)
	{
		m_EditorCamera->OnUpdate(ts);
		PackInfo();
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

		/*if (m_CurrentScene) {
			m_CurrentScene->ClearEntities();
		}
		m_CurrentScene->SetSelectedEntity({});
		SceneSerializer serializer(m_CurrentScene);  // 目前打开一个场景，就是把当前场景清空，加载新场景的Entity
		serializer.Deserialize(filepath.string());
		m_CurrentSceneFilePath = filepath.string();
		std::replace(m_CurrentSceneFilePath.begin(), m_CurrentSceneFilePath.end(), '\\', '/');
		*/
		m_CurrentScene = std::make_shared<Scene>(); 
		SceneSerializer serializer(m_CurrentScene);
        serializer.Deserialize(filepath.string());
		std::filesystem::path path = filepath;
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

	std::pair<unsigned int, unsigned int> SceneManager::GetViewportSize()
	{
		return { m_EditorCamera->GetViewportWidth(),m_EditorCamera->GetViewportHeight() };
	}
}