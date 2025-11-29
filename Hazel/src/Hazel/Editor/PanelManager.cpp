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
	
	void PanelManager::SetScene(std::shared_ptr<Scene> activeScene) {
	}

	void PanelManager::Tick(float deltaTime)
	{
		Begin();
		// ImGuiCommand();
		End();
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
					Application::GetSceneManager()->GetActiveScene() = std::make_shared<Scene>();
					m_AssetManagerPanel.SetContext(Application::GetSceneManager()->GetActiveScene());
					m_FolderPreviewPanel.SetContext(Application::GetSceneManager()->GetActiveScene());
				}

				if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
				{
					Application::GetSceneManager()->OpenScene();
					m_AssetManagerPanel.ClearState();
					m_AssetManagerPanel.SetContext(Application::GetSceneManager()->GetActiveScene());
					m_FolderPreviewPanel.SetContext(Application::GetSceneManager()->GetActiveScene());
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
	bool PanelManager::ImGuiCommand(RHIDescriptorSetRef viewportTexture, RHIDescriptorSetRef debugTexture)
	{
		
		GlobalWindow();
		ViewportGUI(viewportTexture);
		SettingGUI();
		DebugTexture(debugTexture);
		GPUTime();
		DrawGPUProfiler();
		m_FolderPreviewPanel.OnImGuiRender();
		m_AssetManagerPanel.OnImGuiRender();
		m_LogPanel.OnImGuiRender();
		m_RDGPanel.OnImGuiRender();

		return m_FolderPreviewPanel.createModel;
	}
	void PanelManager::ViewportGUI(RHIDescriptorSetRef viewportTexture)
	{
		ImGui::Begin("Viewport");

		m_ViewportBounds[0] = ImGui::GetWindowPos();
		// 程序中ViewportSize都来自这里
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportBounds[1] = ImVec2(m_ViewportBounds[0].x + viewportSize.x, m_ViewportBounds[0].y + viewportSize.y);
		Application::GetSceneManager()->GetEditorCamera()->SetViewportSize(viewportSize.x, viewportSize.y);
		Application::GetSceneManager()->GetActiveScene()->SetViewprotSize(viewportSize.x, viewportSize.y);

		ImVec2 mousePos = ImGui::GetIO().MousePos;
		isMouseInViewport =mousePos.x >= m_ViewportBounds[0].x && mousePos.x <= m_ViewportBounds[1].x && mousePos.y >= m_ViewportBounds[0].y && mousePos.y <= m_ViewportBounds[1].y;
		// LOG_INFO("{0},{1},{2}", mousePos.x, mousePos.y, isMouseInViewport);
		Application::GetSceneManager()->GetEditorCamera()->SetIsMouseInViewPort(isMouseInViewport);


		// 设置ViewPort图片
		ImGui::Image(viewportTexture->RawHandle(), ImGui::GetContentRegionAvail(), {0, 1}, {1, 0});
		// Application::GetSceneManager()->GetActiveScene()->OutputViewport();


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
		// ------------------ 悬浮按钮 ------------------
		ImGui::SetNextWindowPos(ImVec2(m_ViewportBounds[0].x + 5, m_ViewportBounds[0].y + 25));
		ImGui::SetNextWindowSize(ImVec2(100, 40));
		ImGuiWindowFlags floatButtonFlags = ImGuiWindowFlags_NoDecoration |ImGuiWindowFlags_AlwaysAutoResize |ImGuiWindowFlags_NoBackground |ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoFocusOnAppearing;
		ImGui::Begin("SelectionModeUI", nullptr, floatButtonFlags);
		if (ImGui::Button(isSubMeshModelClickModel ? "Submesh" : "Model")) {
			isSubMeshModelClickModel = !isSubMeshModelClickModel;
		}
		ImGui::End();
		// ------------------ 悬浮按钮结束 ------------------

		DrawGizmo();

		ImGui::End();
	}

	void PanelManager::DrawGizmo()
	{
		auto & m_SelectedEntity = Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity();
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
		const glm::mat4& cameraProjection = Application::GetSceneManager()->GetEditorCamera()->GetProjectionMatrix();
		glm::mat4 cameraView = Application::GetSceneManager()->GetEditorCamera()->GetViewMatrix();
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

			glm::vec3 deltaRotation = rotation - tc.GetRotationEuler();
			tc.Translation = translation;
			tc.SetRotation(tc.GetRotationEuler() += deltaRotation);
			tc.Scale = scale;
		}
	}

	void PanelManager::SettingGUI()
	{
		ImGui::Begin("Setting");
		if (ImGui::CollapsingHeader("Test", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button("Hit Me!")) {
				Application::GetSceneManager()->GetActiveScene()->testButton();
			}
		}

		// 阴影设置栏目
		if (ImGui::CollapsingHeader("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 阴影类型选择
			ImGui::RadioButton("Hard Shadow", &Application::GetSceneManager()->GetActiveScene()
				->GetRenderSettingData().ShadowType, 0);
			ImGui::RadioButton("PCF", &Application::GetSceneManager()->GetActiveScene()
				->GetRenderSettingData().ShadowType, 1);
			ImGui::RadioButton("PCSS", &Application::GetSceneManager()->GetActiveScene()
				->GetRenderSettingData().ShadowType, 2);
			int& deBugCSM = Application::GetSceneManager()->GetActiveScene()
				->GetRenderSettingData().deBugCSM;

			// 临时 bool
			bool tmp = (deBugCSM != 0);
			if (ImGui::Checkbox("Show Cascade", &tmp))
			{
				deBugCSM = tmp ? 1 : 0; // 用户点击后更新 int
			}
		}

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

			auto & ray = Ray::CastRay(*APP_SCENE_CAMERA, mouseX, mouseY);
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
				if (isSubMeshModelClickModel) {
					Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity(selectionData[0].mesh);
				}
				else {
                    Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity(selectionData[0].Model);
				}
			}
			else {
				Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity({});

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
		m_AssetManagerPanel.SetContext(Application::GetSceneManager()->GetActiveScene());
		m_FolderPreviewPanel.SetContext(Application::GetSceneManager()->GetActiveScene());
		m_GizmoType = -1;
	}

	void PanelManager::SetGPUTimeInfo(std::vector<RHIGPUTimeInfo>& timeInfo)
	{
		m_GPUTimeInfo = timeInfo;
	}

	void PanelManager::GPUTime()
	{
		if (ImGui::Begin("GPU Timing"))
		{
			ImGui::Text("Per-pass GPU durations (ms)");
			ImGui::Separator();

			if (m_GPUTimeInfo.empty())
			{
				ImGui::Text("No GPU timing data.");
			}
			else
			{
				if (ImGui::BeginTable("GPU Time Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
				{
					ImGui::TableSetupColumn("Pass");
					ImGui::TableSetupColumn("Time (ms)");
					ImGui::TableHeadersRow();

					for (const auto& info : m_GPUTimeInfo)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::TextUnformatted(info.Name.c_str());

						ImGui::TableSetColumnIndex(1);
						ImGui::Text("%.3f", info.DurationMs);
					}

					ImGui::EndTable();
				}
			}
		}

		ImGui::End();
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
	namespace ed = ax::NodeEditor;

	void PanelManager::DrawGPUProfiler() {
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

		// 树状结构开始
		if (ImGui::TreeNode("Passes"))
		{
			const float barMaxWidth = 200.0f;

			for (auto& info : m_GPUTimeInfo)
			{
				// pass 节点
				if (ImGui::TreeNode(info.Name.c_str()))
				{
					float percent = info.DurationMs / totalFrameMs;
					ImVec4 color = GetColorForDuration(info.DurationMs, totalFrameMs);

					ImGui::Text("Time: %.3f ms (%.1f%%)", info.DurationMs, percent * 100.0f);

					// 条状图背景色
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);

					// 绘制条状图
					ImGui::ProgressBar(
						percent,
						ImVec2(barMaxWidth, 0.0f),
						""
					);

					ImGui::PopStyleColor();

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}

}
