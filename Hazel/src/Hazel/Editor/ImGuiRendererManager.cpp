#include "hzpch.h"
#include "Hazel/Core/Base.h"
#include "ImGuiRendererManager.h"
#include "Hazel/Renderer/old/Renderer.h"
#include "Hazel/Platform/Vulkan/VulkanImGuiLayer.h"
#include "Hazel/Renderer/old/RendererAPI.h"
#include <imgui.h>
#include "Hazel/Renderer/old/EditorCamera.h"
#include "Hazel/Core/Input.h"
#include <glm/gtc/type_ptr.hpp>
#include <Hazel/Math/ray.h>
#include "Hazel/Asset/Model/Mesh.h"
#include <ImGuizmo.h>
#include "Hazel/Scene/SceneManager.h"

namespace GameEngine {
	namespace Colors
	{
		// To experiment with editor theme live you can change these constexpr into static
		// members of a static "Theme" class and add a quick ImGui window to adjust the colour values
		namespace Theme
		{
			constexpr auto accent = IM_COL32(236, 158, 36, 255);
			constexpr auto highlight = IM_COL32(39, 185, 242, 255);
			constexpr auto niceBlue = IM_COL32(83, 232, 254, 255);
			constexpr auto compliment = IM_COL32(78, 151, 166, 255);
			constexpr auto background = IM_COL32(36, 36, 36, 255);
			constexpr auto backgroundDark = IM_COL32(26, 26, 26, 255);
			constexpr auto titlebar = IM_COL32(21, 21, 21, 255);
			constexpr auto titlebarOrange = IM_COL32(186, 66, 30, 255);
			constexpr auto titlebarGreen = IM_COL32(18, 88, 30, 255);
			constexpr auto titlebarRed = IM_COL32(185, 30, 30, 255);
			constexpr auto propertyField = IM_COL32(15, 15, 15, 255);
			constexpr auto text = IM_COL32(192, 192, 192, 255);
			constexpr auto textBrighter = IM_COL32(210, 210, 210, 255);
			constexpr auto textDarker = IM_COL32(128, 128, 128, 255);
			constexpr auto textError = IM_COL32(230, 51, 51, 255);
			constexpr auto muted = IM_COL32(77, 77, 77, 255);
			constexpr auto groupHeader = IM_COL32(47, 47, 47, 255);
			constexpr auto selection = IM_COL32(237, 192, 119, 255);
			constexpr auto selectionMuted = IM_COL32(237, 201, 142, 23);
			constexpr auto backgroundPopup = IM_COL32(50, 50, 50, 255);
			constexpr auto validPrefab = IM_COL32(82, 179, 222, 255);
			constexpr auto invalidPrefab = IM_COL32(222, 43, 43, 255);
			constexpr auto missingMesh = IM_COL32(230, 102, 76, 255);
			constexpr auto meshNotSet = IM_COL32(250, 101, 23, 255);
		}
	}
	std::shared_ptr<ImGuiRendererManager> ImGuiRendererManager::Create()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPI::Type::None:    return nullptr;
			case RendererAPI::Type::Vulkan:  return std::make_shared<VulkanImGuiLayer>();
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	void ImGuiRendererManager::SetScene(std::shared_ptr<Scene> activeScene) {
	}

	void ImGuiRendererManager::Tick(float deltaTime)
	{
		Begin();
		// ImGuiCommand();
		End();
	}


	void ImGuiRendererManager::ImGuiCommand(RHIDescriptorSetRef viewportTexture)
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

		// ========== 其他面板绘制 ==========
		ViewportGUI(viewportTexture);
		SettingGUI();
		DebugTexture();
		GPUTime();
		DrawGPUProfiler();
		if (m_FolderPreviewPanel.isOpen)
			m_FolderPreviewPanel.OnImGuiRender();

		if (m_AssetManagerPanel.isOpen)
			m_AssetManagerPanel.OnImGuiRender();

		if (m_LogPanel.isOpen)
			m_LogPanel.OnImGuiRender();

		if (m_RDGPanel.isOpen)
			m_RDGPanel.OnImGuiRender();
	}
	void ImGuiRendererManager::ViewportGUI(RHIDescriptorSetRef viewportTexture)
	{
		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoBackground);

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
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		ImVec4 border_col = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		ImGui::Image(viewportTexture->RawHandle(), ImGui::GetContentRegionAvail(), {0, 1}, {1, 0},tint_col,border_col);

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
		DrawGizmo();

		ImGui::End();
	}

	void ImGuiRendererManager::DrawGizmo()
	{
		m_SelectedEntity = m_AssetManagerPanel.GetSelectedEntity();
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
		glm::mat4 transform = tc.GetTransform();
		bool snap = Input::IsKeyDown(Key::LeftControl);
		float snapValue = (m_GizmoType == ImGuizmo::ROTATE) ? 45.0f : 0.5f;
		float snapValues[3] = { snapValue, snapValue, snapValue };

		ImGuizmo::Manipulate(glm::value_ptr(cameraView),
			glm::value_ptr(cameraProjection),
			(ImGuizmo::OPERATION)m_GizmoType,
			ImGuizmo::LOCAL,
			glm::value_ptr(transform),
			nullptr,
			snap ? snapValues : nullptr);

		if (ImGuizmo::IsUsing())
		{
			glm::vec3 translation, rotation, scale;
			Math::DecomposeTransform(transform, translation, rotation, scale);

			glm::vec3 deltaRotation = rotation - tc.GetRotationEuler();
			tc.Translation = translation;
			tc.SetRotation(tc.GetRotationEuler() += deltaRotation);
			tc.Scale = scale;
		}
	}

	void ImGuiRendererManager::SettingGUI()
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

	bool ImGuiRendererManager::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		if (event.GetMouseButton() != HZ_MOUSE_BUTTON_LEFT)
			return false;
		if (ImGuizmo::IsOver())
			return false;
		auto [mouseX, mouseY] = GetMouseViewportSpace();
		if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f) {
			std::vector<SelectionData> selectionData;

			auto [origin, direction] = CastRay(*Application::GetSceneManager()->GetEditorCamera(), mouseX, mouseY);
			auto meshEntities = Application::GetSceneManager()->GetActiveScene()
				->GetAllEntitiesWith<ModelComponent>();
			for (auto e : meshEntities) {
				Entity entity = { e, Application::GetSceneManager()->GetActiveScene().get()};
				auto& mc = entity.GetComponent<ModelComponent>();
				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mc.StaticMesh);
				auto& submeshes = meshSource->GetSubmeshes();
				for (uint32_t i = 0; i < submeshes.size(); i++)
				{
					auto& submesh = submeshes[i];
					glm::mat4 transform = Application::GetSceneManager()->GetActiveScene()
						->GetWorldSpaceTransformMatrix(entity);
					Ray ray = {
						glm::inverse(transform * submesh.Transform) * glm::vec4(origin, 1.0f),
						glm::inverse(glm::mat3(transform * submesh.Transform)) * direction
					};

					float t;
					bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
					if (intersects)
					{
						const auto& triangleCache = meshSource->GetTriangleCache(i);
						for (const auto& triangle : triangleCache)
						{
							if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
							{
								selectionData.push_back({ entity, &submesh,meshSource, t });
								break;
							}
						}
					}
				}
			}
			auto dynamicMeshEntities = Application::GetSceneManager()->GetActiveScene()
				->GetAllEntitiesWith<DynamicModelComponent>();

			for (auto e : dynamicMeshEntities) {
				Entity entity = { e, 	Application::GetSceneManager()->GetActiveScene().get() };
				auto& mc = entity.GetComponent<DynamicModelComponent>();
				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mc.meshSource);
				auto& submeshes = meshSource->GetSubmeshes();
				for (uint32_t i = 0; i < submeshes.size(); i++)
				{
					auto& submesh = submeshes[i];
					glm::mat4 transform = Application::GetSceneManager()->GetActiveScene()
						->GetWorldSpaceTransformMatrix(entity);
					Ray ray = {
						glm::inverse(transform * submesh.Transform) * glm::vec4(origin, 1.0f),
						glm::inverse(glm::mat3(transform * submesh.Transform)) * direction
					};

					float t;
					bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
					if (intersects)
					{
						const auto& triangleCache = meshSource->GetTriangleCache(i);
						for (const auto& triangle : triangleCache)
						{
							if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
							{
								selectionData.push_back({ entity, &submesh,meshSource, t });
								break;
							}
						}
					}
				}
			}
			if (selectionData.size() > 0) {
				std::sort(selectionData.begin(), selectionData.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });
				LOG_TRACE("Selected Entity: {}", selectionData[0].Entity.GetComponent<TagComponent>().Tag);
				m_SelectedEntity = selectionData[0].Entity;
				m_AssetManagerPanel.SetSelectedEntity(selectionData[0].Entity);
			}
		}

		return false;
	}
	std::pair<float, float> ImGuiRendererManager::GetMouseViewportSpace()  // NDC坐标
	{
		auto [mx, my] = ImGui::GetMousePos();
		const auto& viewportBounds = m_ViewportBounds;
		mx -= viewportBounds[0].x;
		my -= viewportBounds[0].y;
		auto viewportWidth = viewportBounds[1].x - viewportBounds[0].x;
		auto viewportHeight = viewportBounds[1].y - viewportBounds[0].y;

		return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
	}
	std::pair<glm::vec3, glm::vec3> ImGuiRendererManager::CastRay(EditorCamera& camera, float mx, float my)
	{
		glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

		auto inverseProj = glm::inverse(camera.GetProjectionMatrix());
		auto inverseView = glm::inverse(glm::mat3(camera.GetViewMatrix()));

		glm::vec4 ray = inverseProj * mouseClipPos;
		glm::vec3 rayPos = camera.GetPosition();
		glm::vec3 rayDir = inverseView * glm::vec3(ray);

		return { rayPos, rayDir };
	}

	void ImGuiRendererManager::DebugTexture()
	{
		ImGui::Begin("Debug Texture");
		Application::GetSceneManager()->GetActiveScene()->ShowDebugTexture();
		ImGui::End();
	}

	void ImGuiRendererManager::SetDarkThemeV2Colors()
	{
		auto& style = ImGui::GetStyle();
		auto& colors = ImGui::GetStyle().Colors;

		//========================================================
		/// Colours

		// Headers
		colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);

		// Buttons
		colors[ImGuiCol_Button] = ImColor(56, 56, 56, 200);
		colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
		colors[ImGuiCol_ButtonActive] = ImColor(56, 56, 56, 150);

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
		colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);
		colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::propertyField);

		// Tabs
		colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TabHovered] = ImColor(255, 225, 135, 30);
		colors[ImGuiCol_TabActive] = ImColor(255, 225, 135, 60);
		colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

		// Title
		colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Resize Grip
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

		// Check Mark
		colors[ImGuiCol_CheckMark] = ImColor(200, 200, 200, 255);

		// Slider
		colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);

		// Text
		colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

		// Checkbox
		colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::text);

		// Separator
		colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);
		colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::highlight);
		colors[ImGuiCol_SeparatorHovered] = ImColor(39, 185, 242, 150);

		// Window Background
		// colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::titlebar);
		colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::background);
		colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundPopup);
		colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::groupHeader);
		colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(Colors::Theme::backgroundDark);

		// Menubar
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };

		//========================================================
		/// Style
		style.FrameRounding = 2.5f;
		style.FrameBorderSize = 1.0f;
		style.IndentSpacing = 11.0f;
	}

	bool ImGuiRendererManager::OnEvent(Event& e)
	{
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(ImGuiRendererManager::OnMouseButtonPressed));
		return false;
	}

	ImGuiRendererManager::ImGuiRendererManager()
	{
		m_AssetManagerPanel.SetContext(Application::GetSceneManager()->GetActiveScene());
		m_FolderPreviewPanel.SetContext(Application::GetSceneManager()->GetActiveScene());
		m_SelectedEntity = {};
		m_GizmoType = -1;
	}

	void ImGuiRendererManager::SetGPUTimeInfo(std::vector<RHIGPUTimeInfo>& timeInfo)
	{
		m_GPUTimeInfo = timeInfo;
	}

	void ImGuiRendererManager::GPUTime()
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

	void ImGuiRendererManager::DrawGPUProfiler() {
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
