#pragma once
#include "Hazel/Core/Events/MouseEvent.h"
#include "Panels/AssetManagerPanel.h"
#include "Panels/FolderPreviewPanel.h"
#include "Panels/LogPanel.h"
#include <Hazel/Renderer/RDG/RDGHandle.h>
#include "Panels/RDGPanel.h"
#include "Hazel/Core/Definations.h"
namespace GameEngine {
	class SubMesh;
	class PanelManager
	{
	public:
		PanelManager();
		void ImGuiCommand(RHIDescriptorSetRef viewportTexture, RHIDescriptorSetRef debugTexture);
		void SetGPUTimeInfo(std::vector<RHIGPUTimeInfo>& timeInfo);
		bool OnEvent(Event& e);
		std::pair<float, float> GetMouseViewportSpace();

	private:
		void ViewportGUI(RHIDescriptorSetRef viewportTexture);
		void DispatchViewPortSize();
		void RenderViewPortTools();
		void RenderFPS();
		void DrawGizmo();
		void SettingGUI();
		void DebugTexture();
		void DebugTexture(RHIDescriptorSetRef debugTexture);
		void DrawGPUProfiler();
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		void GlobalWindow();
		void DrawStatistics();

	private:
		// Gizmo's
		int m_GizmoType = -1;
		// 面板
		AssetManagerPanel m_AssetManagerPanel;
		FolderPreviewPanel m_FolderPreviewPanel{ APP_ASSET_PATH };
		ImGuiLogPanel m_LogPanel;
		RDGPanel m_RDGPanel;

		//状态
		ImVec2 m_ViewportBounds[2] = { {0,0},{1216,849} };
		bool isMouseInViewport = false;

		bool isSubMeshModelClickMode = false;
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
