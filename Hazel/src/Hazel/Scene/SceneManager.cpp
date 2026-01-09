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

		m_DefaultEditorCamera = std::make_shared<EditorCamera>(45.0f, Application::Get().GetWindowManager()->GetWindowSize().first, Application::Get().GetWindowManager()->GetWindowSize().second, 0.1f, 1000.0f);
	}

	void SceneManager::PackSettingForRender() {
		// 全局设置
		auto& settings = m_CurrentScene->settings;
		if(settings.onlyIndirectionLight){
			m_SceneInfo.globalSettingInfos.renderSetting.debugDDGI = 1;
		}
		else if(settings.noDDGI){
            m_SceneInfo.globalSettingInfos.renderSetting.debugDDGI = 2;
		}
		else {
            m_SceneInfo.globalSettingInfos.renderSetting.debugDDGI = 0;
		}
		m_SceneInfo.globalSettingInfos.renderSetting.renderBoundingBox = settings.renderBoundingBox? 1 : 0;
		m_SceneInfo.globalSettingInfos.renderSetting.ClusterLightFrustumDebug = settings.ClusterLightFrustumDebug ? 1 : 0;

		// ActiveCamera设置
		auto activeCamera = m_CurrentScene->GetFirstEntityWith<CameraComponent>(); // TODO: 临时，这样写的话，只会判断第一个摄像机组件，不支持多个摄像机
		Entity cameraEntity = Entity{ activeCamera ,m_CurrentScene };
        if (cameraEntity) {
			auto& component = cameraEntity.GetComponent<CameraComponent>();
			if (component.Primary) {
				m_ActiveEditorCamera = component.CameraRef;
			}
			else {
				m_ActiveEditorCamera = m_DefaultEditorCamera;
			}
		}
		else {
			m_ActiveEditorCamera = m_DefaultEditorCamera;
		}

		// 灯光设置
		auto dirLight = m_CurrentScene->GetFirstEntityWith<DirectionalLightComponent>();
		Entity dirLightEntity = Entity{ dirLight ,m_CurrentScene };
		if (dirLightEntity) {
			auto& component = dirLightEntity.GetComponent<DirectionalLightComponent>();
			m_SceneInfo.globalSettingInfos.shadowSetting.ShadowType = settings.DirShadowType;
			m_SceneInfo.globalSettingInfos.shadowSetting.PointShadowType = settings.PointShadowType;
			m_SceneInfo.globalSettingInfos.shadowSetting.DebugCSM = component.showCSM;
			m_SceneInfo.globalSettingInfos.shadowSetting.CSMSmooth = component.CSMSmooth;
		}

		// 后处理设置
		auto postprocess = m_CurrentScene->GetFirstEntityWith<PostProcessingComponent>();
		Entity postProcessEntity = Entity{ postprocess ,m_CurrentScene };
		if (postProcessEntity) {
			auto& component = postProcessEntity.GetComponent<PostProcessingComponent>();
			// Bloom
			m_SceneInfo.globalSettingInfos.postprocess.bloomSetting.bloomScale = component.bloomScale;
			m_SceneInfo.globalSettingInfos.postprocess.bloomSetting.enable = component.enableBloom;

			// TAA
			m_SceneInfo.globalSettingInfos.postprocess.TaaSetting.enable = component.enableTAA;
			m_SceneInfo.globalSettingInfos.postprocess.TaaSetting.shaper = component.taaSharpen;
			m_SceneInfo.globalSettingInfos.postprocess.TaaSetting.shaperStrength = component.taaSharpness;
			m_SceneInfo.globalSettingInfos.postprocess.TaaSetting.UVjetter = HaltonUtils::GetJitter(APP_TICK);
		
			// PathTracing
			m_SceneInfo.globalSettingInfos.postprocess.pathTracingSetting.enable = component.pathTracingEnable;
            m_SceneInfo.globalSettingInfos.postprocess.pathTracingSetting.numSamples = component.pathTracingNumSamples;
            m_SceneInfo.globalSettingInfos.postprocess.pathTracingSetting.numBounce = component.pathTracingNumBounce;
            m_SceneInfo.globalSettingInfos.postprocess.pathTracingSetting.sampleSkyBox = component.pathTracingSampleSkyBox ? 1 : 0;
            m_SceneInfo.globalSettingInfos.postprocess.pathTracingSetting.indirectOnly = component.pathTracingIndirectOnly ? 1 : 0;
            m_SceneInfo.globalSettingInfos.postprocess.pathTracingSetting.historyActive = component.pathTracingHistoryActive ? 1 : 0;

			// Color
            m_SceneInfo.globalSettingInfos.postprocess.colorSetting.exposure = component.exposure;
            m_SceneInfo.globalSettingInfos.postprocess.colorSetting.saturation = component.saturation;
            m_SceneInfo.globalSettingInfos.postprocess.colorSetting.contrast = component.contrast;
            m_SceneInfo.globalSettingInfos.postprocess.colorSetting.toneMappingMode = component.toneMappingMode;
		
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

		// 探针设置
        auto probe = m_CurrentScene->GetFirstEntityWith<LightProbeComponent>();
        Entity probeEntity = Entity{ probe ,m_CurrentScene };
		if (probeEntity) {
			auto& transform = probeEntity.GetComponent<TransformComponent>();
			auto& component = probeEntity.GetComponent<LightProbeComponent>();
			auto& ddgi = m_SceneInfo.globalSettingInfos.ddgiSetting;
			ddgi.centerPosition = transform.Translation;
			ddgi.enable = component.enable ? 1 : 0;
			ddgi.probeCount = component.probeCount;
			ddgi.gridStep = component.gridStep;
			ddgi.raysPerProbe = component.raysPerProbe;

			ddgi.infineBounds = component.infiniteBounds ? 1 : 0;
			ddgi.getSkyLight = component.getSkyLight ? 1 : 0;

			ddgi.visulaize = component.visulaize ? 1 : 0;
			glm::vec3 halfExtent = 0.5f * glm::vec3(component.probeCount) * component.gridStep;
			ddgi.box.minBound = ddgi.centerPosition - halfExtent;
			ddgi.box.maxBound = ddgi.centerPosition + halfExtent;
		}

	}

	void SceneManager::PackInfo() {
		m_SceneInfo = {};
		PackSettingForRender();
		m_SceneInfo.camera = m_ActiveEditorCamera;
		m_SceneInfo.defaultCamera = m_DefaultEditorCamera;
	};

	void SceneManager::Tick(Timestep ts)
	{
		if (m_ActiveEditorCamera) {
			m_ActiveEditorCamera->OnUpdate(ts);
		}
		// m_DefaultEditorCamera->OnUpdate(ts);
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
		return { m_DefaultEditorCamera->GetViewportWidth(),m_DefaultEditorCamera->GetViewportHeight() };
	}
}