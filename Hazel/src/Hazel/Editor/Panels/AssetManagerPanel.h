#pragma once
#include "EditorPanel.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Entity.h"
namespace GameEngine {
	class AssetManagerPanel : public EditorPanel
	{
	public:
		AssetManagerPanel();

		void SetContext(std::shared_ptr<Scene>& context);
		void ClearState() { m_RenameEntity = {}; }
		void OnImGuiRender() override;
		void SetSelectedEntity(Entity entity);
	private:
		void DrawEntityNode(Entity entity);

		void DrawComponents(Entity entity);

	private:
		static void DrawMaterial(UUID meshSourceHandle);

	private:
		std::shared_ptr<Scene> m_Context;
		Entity m_RenameEntity;
		char m_RenameBuffer[256]{};
		IconData m_EntityIcon;
		IconData m_DirLightIcon;
		IconData m_SpotLightIcon;
		IconData m_SkyLightIcon;
		IconData m_PointLightIcon;
		template<typename T, typename UIFunction>
		void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction);
		template<typename T>
		void DisplayAddComponentEntry(const std::string& entryName);
	};
}
