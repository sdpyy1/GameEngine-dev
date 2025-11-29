#pragma once
#include "Hazel/Core/Events/MouseEvent.h"
#include "Panels/AssetManagerPanel.h"
#include "Panels/FolderPreviewPanel.h"
#include "Panels/LogPanel.h"
#include <Hazel/Renderer/RDG/RDGHandle.h>
#include "Panels/RDGPanel.h"
namespace GameEngine {
	class SubMesh;
	class PanelManager
	{
	public:
		PanelManager();
		virtual void Begin() {};
		virtual void End() {};
		void Tick(float deltaTime);
		bool ImGuiCommand(RHIDescriptorSetRef viewportTexture, RHIDescriptorSetRef debugTexture);
		bool OnEvent(Event& e);
		// 各种窗口创建
		void ViewportGUI(RHIDescriptorSetRef viewportTexture);
		void DrawGizmo();
		void SettingGUI();
		void DebugTexture(RHIDescriptorSetRef debugTexture);
		void GPUTime();
		void DrawGPUProfiler();
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		std::pair<float, float> GetMouseViewportSpace(); // NDC坐标
		std::pair<glm::vec3, glm::vec3> CastRay(EditorCamera& camera, float mx, float my);
		void SetScene(std::shared_ptr<Scene> activeScene);
		void SetGPUTimeInfo(std::vector<RHIGPUTimeInfo>& timeInfo);
		void GlobalWindow();
	private:

		// Gizmo's
		int m_GizmoType = -1;
		// 面板
		AssetManagerPanel m_AssetManagerPanel;
		FolderPreviewPanel m_FolderPreviewPanel{ "assets" };
		ImGuiLogPanel m_LogPanel;
        RDGPanel m_RDGPanel;
		//状态
		ImVec2 m_ViewportBounds[2] = { {0,0},{1216,849} };
		bool isMouseInViewport = false;
		bool firstRenderGUI = true;

		bool isSubMeshModelClickModel = false; // false 表示选择整个模型，true 表示选择Submesh
		struct SelectionData
		{
			Entity Model;
			Entity mesh;
			float Distance = 0.0f;
		};
		// 数据
		std::vector<RHIGPUTimeInfo> m_GPUTimeInfo;
	};

}
