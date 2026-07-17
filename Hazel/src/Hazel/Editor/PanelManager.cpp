#include "hzpch.h"
#include "Hazel/Core/Base.h"
#include "PanelManager.h"
#include <imgui.h>
#include "Hazel/Core/Input.h"
#include <glm/gtc/type_ptr.hpp>
#include <Hazel/Math/ray.h>
#include <ImGuizmo.h>
#include "Hazel/Scene/SceneManager.h"

namespace GameEngine {
	void PanelManager::ImGuiCommand(RHIDescriptorSetRef viewportTexture, RHIDescriptorSetRef debugTexture)
	{
		GlobalWindow();
		ViewportGUI(viewportTexture);
		SettingGUI();
		DebugTexture(debugTexture);
		DrawGPUProfiler();
		DrawStatistics();
		m_FolderPreviewPanel.OnImGuiRender();
		m_AssetManagerPanel.OnImGuiRender();
		m_LogPanel.OnImGuiRender();
		// m_RDGPanel.OnImGuiRender();   // 一个玩意占20FPS
	}
	void PanelManager::ViewportGUI(RHIDescriptorSetRef viewportTexture)
	{
		ImGui::Begin("Viewport");
		DispatchViewPortSize();
		ImGui::Image(viewportTexture->RawHandle(), ImGui::GetContentRegionAvail(), { 0, 1 }, { 1, 0 });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				IM_ASSERT(payload->DataSize > 0);
				const char* droppedPath = (const char*)payload->Data;
				Application::GetSceneManager()->OpenScene(droppedPath);
			}
			ImGui::EndDragDropTarget();
		}

		RenderViewPortTools();
		RenderFPS();
		DrawGizmo();

		ImGui::End();
	}
	void PanelManager::DrawStatistics()
	{
		ImGui::Begin("Statistics");
		ImGui::Text("CPU DrawCall: %u", APP_RENDERSYSTEM->GetDrawCallCount());
		ImGui::Text("Mesh DrawCall: %u", APP_RENDERSYSTEM->GetDrawMeshCount());
		// ImGui::Text("GPU DrawCall: %u", APP_RENDERSYSTEM->GetGPUDrawCallCount());

		ImGui::End();
	}
	void PanelManager::DrawGizmo()
	{
		auto& m_SelectedEntity = Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity();
		bool rightMouseDown = ImGui::IsMouseDown((int)Button::Right);
		if (rightMouseDown) {
			m_GizmoType = -1;
		}
		else if (!ImGuizmo::IsUsing()) {
			if (Input::IsKeyDown(Key::W)) m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
			if (Input::IsKeyDown(Key::E)) m_GizmoType = ImGuizmo::OPERATION::ROTATE;
			if (Input::IsKeyDown(Key::R)) m_GizmoType = ImGuizmo::OPERATION::SCALE;
			if (Input::IsKeyDown(Key::Q)) m_GizmoType = -1;
		}
		if (!m_SelectedEntity || m_GizmoType == -1)
			return;

		if (!isMouseInViewport)
			return;
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		float x = m_ViewportBounds[0].x;
		float y = m_ViewportBounds[0].y;
		float width = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
		float height = m_ViewportBounds[1].y - m_ViewportBounds[0].y;
		ImGuizmo::SetRect(x, y, width, height);
		const glm::mat4& cameraProjection = Application::GetSceneManager()->GetActiveEditorCamera()->GetProjectionMatrix();
		glm::mat4 cameraView = Application::GetSceneManager()->GetActiveEditorCamera()->GetViewMatrix();
		auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
		glm::mat4 transform = APP_SCENEMANAGER->GetActiveScene()->GetWorldSpaceTransformMatrix(m_SelectedEntity);
		bool snap = Input::IsKeyDown(Key::LeftControl);
		float snapValue = (m_GizmoType == ImGuizmo::ROTATE) ? 45.0f : 0.5f;
		float snapValues[3] = { snapValue, snapValue, snapValue };

		ImGuizmo::Manipulate(glm::value_ptr(cameraView),
			glm::value_ptr(cameraProjection),
			(ImGuizmo::OPERATION)m_GizmoType,
			ImGuizmo::LOCAL,
			glm::value_ptr(transform), // transform会被ImGuizmo修改
			nullptr,
			snap ? snapValues : nullptr);

		if (ImGuizmo::IsUsing())
		{
			auto newLocalTrans = APP_SCENEMANAGER->GetActiveScene()->GetLocalTransformMatrix(m_SelectedEntity, transform);
			glm::vec3 translation, rotation, scale;
			Math::DecomposeTransform(newLocalTrans, translation, rotation, scale);
			tc.Translation = translation;
			tc.SetRotation(rotation);
			tc.Scale = scale;
		}
	}

	void PanelManager::SettingGUI()
	{
		ImGui::Begin("Setting");
		auto& curScene = APP_SCENEMANAGER->GetActiveScene();
		if (ImGui::CollapsingHeader("DDGI", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("OnlyDDGI", &curScene->settings.onlyIndirectionLight);
			ImGui::Checkbox("NoDDGI", &curScene->settings.noDDGI);
		}
		if (ImGui::CollapsingHeader("RenderSetting", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("ShowBoundingBox", &curScene->settings.renderBoundingBox);
			ImGui::Checkbox("FrustumDebug", &curScene->settings.ClusterLightFrustumDebug);
		}
		if (ImGui::CollapsingHeader("ShadowSetting", ImGuiTreeNodeFlags_DefaultOpen)) {
			const char* dirShadowTypeNames[] = {
					"None",
					"Hard Shadow",
					"PCF",
					"PCSS"
			};
			int currentDirShadowType = static_cast<int>(curScene->settings.DirShadowType);
			if (ImGui::Combo("Direction Shadow Type", &currentDirShadowType, dirShadowTypeNames, IM_ARRAYSIZE(dirShadowTypeNames))) {
				curScene->settings.DirShadowType = static_cast<ShadowType>(currentDirShadowType);
			}


			const char* pointShadowTypeNames[] = {
				"None",
				"Hard Shadow",
				"PCF(No Impl)",
				"PCSS(No Impl)",
				"VSM",
				"EVSM"
			};
			int currentPointShadowType = static_cast<int>(curScene->settings.PointShadowType);
			if (ImGui::Combo("Point Shadow Type", &currentPointShadowType, pointShadowTypeNames, IM_ARRAYSIZE(pointShadowTypeNames))) {
				curScene->settings.PointShadowType = static_cast<ShadowType>(currentPointShadowType);
			}
		}
		DebugTexture();
		ImGui::End();
	}
	bool PanelManager::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		if (event.GetMouseButton() != HZ_MOUSE_BUTTON_LEFT)
			return false;
		if (ImGuizmo::IsOver())
			return false;
		auto [mouseX, mouseY] = GetMouseViewportSpace();
		if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f) {
			std::vector<SelectionData> selectionData;

			auto& ray = Ray::CastRay(*APP_SCENE_CAMERA, mouseX, mouseY);
			auto meshEntities = Application::GetSceneManager()->GetActiveScene()->GetAllEntitiesWith<ModelComponent>();

			for (auto entity : meshEntities)
			{
				Entity curEntity = { entity,APP_SCENEMANAGER->GetActiveScene() };

				for (auto childUUID : curEntity.Children()) {
					Entity childEntity = APP_SCENEMANAGER->GetActiveScene()->GetEntityByUUID(childUUID);
					if (childEntity && childEntity.HasComponent<SubmeshComponent>()) {
						auto& model = childEntity.GetComponent<SubmeshComponent>().model;
						auto& mesh = childEntity.GetComponent<SubmeshComponent>().GetMesh();
						auto& originAABB = mesh->aabb;
						glm::mat4 modelMatrix = APP_SCENEMANAGER->GetActiveScene()->GetWorldSpaceTransformMatrix(childEntity);
						auto& transformedABB = originAABB.Transformed(modelMatrix);
						float t;
						if (ray.IntersectsAABB(transformedABB, t)) {
							LOG_INFO("Click Hit Model {} subMesh {}", model->GetPath(), mesh->name);
							selectionData.push_back({ curEntity,childEntity,t });
						}
					}
				}
			}

			if (selectionData.size() > 0) {
				std::sort(selectionData.begin(), selectionData.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });
				LOG_TRACE("Selected Entity: {}", selectionData[0].Model.GetComponent<TagComponent>().Tag);
				m_GizmoType = 0;
				if (isSubMeshModelClickMode) {
					Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity(selectionData[0].mesh);
				}
				else {
					Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity(selectionData[0].Model);
				}
			}
			else {
				// Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity({});
			}
		}

		return false;
	}
	std::pair<float, float> PanelManager::GetMouseViewportSpace()  // NDC坐标
	{
		auto [mx, my] = ImGui::GetMousePos();
		const auto& viewportBounds = m_ViewportBounds;
		mx -= viewportBounds[0].x;
		my -= viewportBounds[0].y;
		auto viewportWidth = viewportBounds[1].x - viewportBounds[0].x;
		auto viewportHeight = viewportBounds[1].y - viewportBounds[0].y;

		return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
	}

	void PanelManager::DebugTexture(RHIDescriptorSetRef debugTexture)
	{
		ImGui::Begin("Debug Texture");
		ImGui::Image(debugTexture->RawHandle(), ImGui::GetContentRegionAvail(), { 0, 1 }, { 1, 0 });
		ImGui::End();
	}

	bool PanelManager::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(PanelManager::OnMouseButtonPressed));
		return false;
	}

	PanelManager::PanelManager()
	{
		m_GizmoType = -1;
	}

	void PanelManager::SetGPUTimeInfo(std::vector<RHIGPUTimeInfo>& timeInfo)
	{
		m_GPUTimeInfo = timeInfo;
	}
	ImVec4 GetColorForDuration(float ms, float maxMs)
	{
		float t = ms / maxMs;
		// Green → Yellow → Red
		return ImVec4(
			t < 0.5f ? 0.0f : (t - 0.5f) * 2.0f,   // Red
			t < 0.5f ? t * 2.0f : 1.0f,            // Green
			0.0f,
			1.0f
		);
	}
	void PanelManager::DrawGPUProfiler()
	{
		ImGui::Begin("GPU Profiler");

		if (m_GPUTimeInfo.empty())
		{
			ImGui::TextDisabled("No GPU timing data captured.");
			ImGui::End();
			return;
		}

		// 计算总帧 GPU 耗时
		float totalFrameMs = 0.0f;
		for (auto& info : m_GPUTimeInfo)
			totalFrameMs += info.DurationMs;

		ImGui::Text("Total GPU Frame Time: %.3f ms", totalFrameMs);
		ImGui::Separator();

		// 表格 + 条状图
		if (ImGui::BeginTable("GPU Time Table", 3,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
		{
			ImGui::TableSetupColumn("Pass");
			ImGui::TableSetupColumn("Time (ms)");
			ImGui::TableSetupColumn("Usage");

			ImGui::TableHeadersRow();

			const float barMaxWidth = 200.0f;

			for (auto& info : m_GPUTimeInfo)
			{
				float percent = info.DurationMs / totalFrameMs;
				ImVec4 color = GetColorForDuration(info.DurationMs, totalFrameMs);

				ImGui::TableNextRow();

				// Pass 名称
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(info.Name.c_str());

				// 耗时
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.3f", info.DurationMs);

				// 条状图显示比例
				ImGui::TableSetColumnIndex(2);
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
				ImGui::ProgressBar(percent, ImVec2(barMaxWidth, 0.0f), nullptr);
				ImGui::PopStyleColor();
			}

			ImGui::EndTable();
		}

		ImGui::End();
	}

	void PanelManager::RenderViewPortTools() {
		// ------------------ 悬浮按钮 ------------------
		ImGui::SetNextWindowPos(ImVec2(m_ViewportBounds[0].x + 5, m_ViewportBounds[0].y + 25));
		ImGui::SetNextWindowSize(ImVec2(100, 40));
		ImGuiWindowFlags floatButtonFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing;
		ImGui::Begin("SelectionModeUI", nullptr, floatButtonFlags);
		if (ImGui::Button(isSubMeshModelClickMode ? "Submesh" : "Model")) {
			isSubMeshModelClickMode = !isSubMeshModelClickMode;
		}
		ImGui::End();
		// ------------------ 悬浮按钮结束 ------------------
	}

	void PanelManager::RenderFPS()
	{
		//------------------------------------------------------
		//  FPS Overlay（强制画在 Viewport 之上）
		//------------------------------------------------------

		// 设置浮窗位置：右上角
		const float padding = 20.0f;
		ImVec2 fpsPos(
			m_ViewportBounds[1].x - 70.0f,
			m_ViewportBounds[0].y + padding
		);
		ImGui::SetNextWindowPos(fpsPos);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
		ImGuiWindowFlags fpsFlags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoBackground;
		ImGui::Begin("FPSOverlay", nullptr, fpsFlags);
		ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
		ImGui::Text("Time: %.2f", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::End();

		ImGui::PopStyleColor();
		//------------------------------------------------------
	}

	void PanelManager::DispatchViewPortSize()
	{
		// 程序中ViewportSize都来自这里
		m_ViewportBounds[0] = ImGui::GetWindowPos();

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportBounds[1] = ImVec2(m_ViewportBounds[0].x + viewportSize.x, m_ViewportBounds[0].y + viewportSize.y);
		Application::GetSceneManager()->GetActiveEditorCamera()->SetViewportSize(viewportSize.x, viewportSize.y);
		Application::GetSceneManager()->GetActiveScene()->SetViewprotSize(viewportSize.x, viewportSize.y);
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		isMouseInViewport = mousePos.x >= m_ViewportBounds[0].x && mousePos.x <= m_ViewportBounds[1].x && mousePos.y >= m_ViewportBounds[0].y && mousePos.y <= m_ViewportBounds[1].y;
		Application::GetSceneManager()->GetActiveEditorCamera()->SetIsMouseInViewPort(isMouseInViewport);
	}
	void PanelManager::GlobalWindow()
	{
		// ========== Dockspace 主窗口 ==========
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		window_flags |= ImGuiWindowFlags_MenuBar; // 为菜单栏预留空间

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGui::Begin("Main Window", nullptr, window_flags);

		// ========== 顶部菜单栏 ==========
		if (ImGui::BeginMenuBar())
		{
			// --- File 菜单 ---
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
				{
					Application::GetSceneManager()->SetActiveScene(std::make_shared<Scene>());
				}

				if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
				{
					Application::GetSceneManager()->OpenScene();
					m_AssetManagerPanel.ClearState();
				}

				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
				{
					Application::GetSceneManager()->SaveScene();
				}

				ImGui::Separator();
				if (ImGui::MenuItem("Exit"))
					Application::Get().Close();

				ImGui::EndMenu();
			}

			// --- View 菜单 ---
			if (ImGui::BeginMenu("View"))
			{
				ImGui::MenuItem("Folder Preview", nullptr, &m_FolderPreviewPanel.isOpen);
				ImGui::MenuItem("Asset Manager", nullptr, &m_AssetManagerPanel.isOpen);
				ImGui::EndMenu();
			}

			// --- Help 菜单 ---
			if (ImGui::BeginMenu("Help"))
			{
				ImGui::Text("GameEngine Editor - Custom Build");
				ImGui::Separator();
				ImGui::Text("Ctrl+S  Save Scene");
				ImGui::Text("W/E/R   Gizmo Control");
				ImGui::Text("Right Click  Free Look");
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		// ========== DockSpace 区域 ==========
		ImGuiID dockspaceID = ImGui::GetID("MyMainDockspace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), 0);
		ImGui::End(); // Main Window
		ImGui::PopStyleVar(2);
	}

	void PanelManager::DebugTexture()
	{
		if (ImGui::CollapsingHeader("DebugTexture", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& rdgDependencyGraph = APP_RENDERSYSTEM->GetRDGDependenctyGraph();
			if (rdgDependencyGraph) {
				auto& textures = rdgDependencyGraph->GetNodes<RDGTextureNode>();
				static std::string selectedImageName;
				static char filterBuffer[128] = "Path";
				ImGui::Text("Filter:");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(std::max(100.0f, ImGui::GetContentRegionAvail().x - 80.0f));
				ImGui::InputTextWithHint("##TextureFilter", "search...", filterBuffer, IM_ARRAYSIZE(filterBuffer));
				ImGui::Separator();

				ImGui::Text("Available Textures:");
				ImVec2 childSize(ImGui::GetContentRegionAvail().x, std::max(100.0f, 300.0f));
				if (ImGui::BeginChild("##TextureList", childSize, true, ImGuiWindowFlags_HorizontalScrollbar))
				{
					for (auto& texture : textures)
					{
						if (texture->Name().empty())
							continue;

						const std::string& textureName = texture->Name();
						bool matchFilter = true;
						if (strlen(filterBuffer) > 0)
						{
							std::string lowerName = textureName;
							std::string lowerFilter = filterBuffer;
							std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
							std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);
							matchFilter = (lowerName.find(lowerFilter) != std::string::npos);
						}

						if (!matchFilter)
							continue;

						ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
						bool isSelected = (textureName == selectedImageName);

						if (isSelected)
						{
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f)); // 入栈
							nodeFlags |= ImGuiTreeNodeFlags_Selected;
						}

						ImGui::SetNextItemWidth(100.0f);
						if (ImGui::TreeNodeEx(textureName.c_str(), nodeFlags))
						{
							if (ImGui::IsItemClicked())
							{
								selectedImageName = textureName;
								APP_SCENEMANAGER->SetDebugImageName(selectedImageName.c_str());
							}
						}

						if (isSelected)
						{
							ImGui::PopStyleColor();
						}
					}
				}
				ImGui::EndChild();
				ImGui::Separator();
			}
		}
	}
}