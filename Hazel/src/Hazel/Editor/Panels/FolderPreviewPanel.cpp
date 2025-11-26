#include "hzpch.h"
#include "FolderPreviewPanel.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "Hazel/Renderer/old/Renderer.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Asset/Model/Mesh.h"
namespace GameEngine {
	FolderPreviewPanel::FolderPreviewPanel(const std::filesystem::path& assetsDir)
		: m_AssetsDir(assetsDir), m_CurrentDir(assetsDir)
	{
		// 扫描分类（递归）
		ScanAssetsForCategories(assetsDir);

		TextureSpecification spec;
		spec.DebugName = "FolderIcons";
		spec.GenerateMips = false;


		m_DirectoryIcon.LoadIconData("Assets/Icon/DirectoryIcon.png");
        m_FileIcon.LoadIconData("Assets/Icon/FileIcon.png");
        m_ModelIcon.LoadIconData("Assets/Icon/pawn.png");
        m_SceneIcon.LoadIconData("Assets/Icon/scene.png");


		// 默认使用分类模式，并默认选中 Scenes（右侧立即显示）
		m_Mode = BrowserMode::Category;
		m_SelectedCategory = "Scenes";
		m_CategoryPreviewFiles = m_SceneFiles;
		m_SelectedFile.clear();
	}

	void FolderPreviewPanel::SetContext(std::shared_ptr<Scene>& context)
	{
		m_Context = context;
	}

	void FolderPreviewPanel::OnFileOpen(const std::filesystem::path& path)
	{
		std::string ext = path.extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == ".scene" || ext == ".hscene")
		{
			LOG_WARN("Double-click to open Scene is not yet supported. Please drag the file into the scene to use it.");
		}
		else if (ext == ".fbx" || ext == ".gltf" || ext == ".obj")
		{

			ModelRef model = AssetManager::LoadModel(path.string());
			model->OnLoadAsset();
			Entity modelEntity = m_Context->CreateEntity(path.string());

			// StaticModel
			auto& modelComponent =  modelEntity.AddComponent<ModelComponent>(model->GetUID(), path);

			int submeshIndex = 0;
			for (auto& mesh : model->GetSubmeshes()) {
				auto& subMeshEntity = m_Context->CreateEntity(mesh.mesh->name);
				subMeshEntity.SetParent(modelEntity);
				subMeshEntity.AddComponent<SubmeshComponent>(model->GetUID(), submeshIndex++);
			}
			createModel = true;
		}
		else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
		{
			LOG_INFO("Preview texture: {}", path.string());
			m_PreviewTexture.LoadIconData(path.string());
			m_PreviewPath = path;
			m_ShowPreview = true;
		}
	}

	void FolderPreviewPanel::OnImGuiRender()
	{
		createModel = false;
		ImGui::Begin("Content Browser");

		DrawToolbar();
		ImGui::Separator();

		// 左侧面板宽度设置（更窄更实用）
		static float leftWidth = 200.0f;
		const float minLeft = 150.0f;
		const float maxLeft = 320.0f;

		ImGui::BeginChild("LeftPanel", ImVec2(leftWidth, 0), true);

		if (m_Mode == BrowserMode::Directory)
			DrawDirectoryTree(m_AssetsDir);
		else
			DrawCategoryTree();

		ImGui::EndChild();

		// 分割条
		ImGui::SameLine();
		ImGui::InvisibleButton("Splitter", ImVec2(4.0f, -1.0f));
		if (ImGui::IsItemActive())
		{
			leftWidth += ImGui::GetIO().MouseDelta.x;
			leftWidth = std::clamp(leftWidth, minLeft, maxLeft);
		}

		// 右侧文件区域
		ImGui::SameLine();
		ImGui::BeginChild("RightPanel", ImVec2(0, 0), false);
		DrawFileGrid();
		ImGui::EndChild();

		// 弹出图片预览（如果有）
		DrawPreviewWindow();

		ImGui::End();
	}

	void FolderPreviewPanel::DrawFileGrid()
	{
		const float padding = 12.0f;
		const float thumbnailSize = 90.0f;
		const float cellSize = thumbnailSize + padding;

		const float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1) columnCount = 1;

		ImGui::Columns(columnCount, nullptr, false);

		// 根据当前模式准备要显示的文件列表
		std::vector<std::filesystem::path> filesToShow;
		if (m_Mode == BrowserMode::Directory)
		{
			for (auto& entry : std::filesystem::directory_iterator(m_CurrentDir))
				filesToShow.push_back(entry.path());
		}
		else // Category 模式
		{
			filesToShow = m_CategoryPreviewFiles;
		}

		for (auto& path : filesToShow)
		{
			std::string name = path.filename().string();

			ImGui::PushID(name.c_str());
			ImGui::BeginGroup();

			// 图标选择
			IconData icon;
			if (std::filesystem::is_directory(path))
			{
				icon = m_DirectoryIcon; // 文件夹使用目录图标
			}
			else
			{
				std::string ext = path.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

				if (ext == ".fbx" || ext == ".obj" || ext == ".gltf")
					icon = m_ModelIcon;
				else if (ext == ".scene" || ext == ".hscene")
					icon = m_SceneIcon;
				else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
					icon = m_FileIcon; // 贴图使用默认图标
				else
					icon = m_FileIcon;
			}

			// ImageButton 样式
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.5f, 0.7f, 0.45f));

			ImGuiID imageBtnId = ImGui::GetID(("image_btn_" + name).c_str());
			bool clicked = ImGui::ImageButtonEx(
				imageBtnId,
				icon.textureID->RawHandle(),
				ImVec2(thumbnailSize, thumbnailSize),
				ImVec2(0, 1),
				ImVec2(1, 0),
				ImVec2(0, 0),
				ImVec4(0, 0, 0, 0),
				ImVec4(1, 1, 1, 1)
			);

			ImGui::PopStyleColor(3);
			ImGui::SetItemAllowOverlap();

			// 拖拽源
			if (ImGui::BeginDragDropSource())
			{
				std::string absPath = std::filesystem::absolute(path).string();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", absPath.c_str(), absPath.size() + 1);
				ImGui::Image(icon.textureID->RawHandle(), ImVec2(32, 32), ImVec2(0, 1), ImVec2(1, 0));
				ImGui::Text("Dragging: %s", absPath.c_str());
				ImGui::EndDragDropSource();
			}

			// 双击逻辑
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (std::filesystem::is_directory(path))
					m_CurrentDir = path; // 进入目录
				else
					OnFileOpen(path);   // 打开文件
			}

			// 右键菜单
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				ImGui::OpenPopup(("file_popup_" + name).c_str());
			}
			if (ImGui::BeginPopup(("file_popup_" + name).c_str()))
			{
				if (!std::filesystem::is_directory(path) && ImGui::MenuItem("Open"))
				{
					OnFileOpen(path);
				}
				if (ImGui::MenuItem("Show in Explorer"))
				{
					// TODO: 平台相关实现
				}
				ImGui::EndPopup();
			}

			// 文件名
			ImGui::TextWrapped(name.c_str());

			ImGui::EndGroup();
			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::Columns(1);
	}

	void FolderPreviewPanel::DrawPreviewWindow()
	{
		if (!m_ShowPreview)
			return;

		bool open = m_ShowPreview;
		ImGui::Begin("Image Preview", &open, ImGuiWindowFlags_AlwaysAutoResize);

		if (m_PreviewTexture.icon)
		{
			ImGui::Text("Path: %s", m_PreviewPath.filename().string().c_str());
			ImGui::Separator();

			float maxWidth = 512.0f;
			float texWidth = (float)m_PreviewTexture.icon->GetWidth();
			float texHeight = (float)m_PreviewTexture.icon->GetHeight();
			float aspect = texHeight / texWidth;

			ImVec2 size = ImVec2(std::min(texWidth, maxWidth), std::min(texWidth, maxWidth) * aspect);
			ImGui::Image(m_PreviewTexture.textureID->RawHandle(), size);
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Failed to load image!");
		}

		ImGui::End();

		if (!open)
		{
			m_ShowPreview = false;
			/*Renderer::SubmitResourceFree([texture = m_PreviewTexture]() mutable {
				texture = nullptr;
				});*/
			LOG_INFO("Closed preview window, texture destroyed.");
		}
	}

	void FolderPreviewPanel::DrawDirectoryTree(const std::filesystem::path& directory)
	{
		for (auto& entry : std::filesystem::directory_iterator(directory))
		{
			if (!entry.is_directory())
				continue;

			const auto& path = entry.path();
			std::string folderName = path.filename().string();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
			if (path == m_CurrentDir)
				flags |= ImGuiTreeNodeFlags_Selected;

			bool nodeOpen = m_ExpandedFolders.count(path.string()) > 0;

			ImGui::PushID(folderName.c_str());

			ImGui::AlignTextToFramePadding();
			ImGui::Image(m_DirectoryIcon.textureID->RawHandle(), ImVec2(16, 16), ImVec2(0, 1), ImVec2(1, 0));
			ImGui::SameLine();
			bool clicked = ImGui::Selectable(folderName.c_str(), path == m_CurrentDir, ImGuiSelectableFlags_SpanAvailWidth);

			if (clicked)
			{
				m_CurrentDir = path;
				if (nodeOpen)
					m_ExpandedFolders.erase(path.string());
				else
					m_ExpandedFolders.insert(path.string());
			}

			if (nodeOpen)
			{
				ImGui::Indent(20.0f);
				DrawDirectoryTree(path);
				ImGui::Unindent(20.0f);
			}

			ImGui::PopID();
		}
	}

	void FolderPreviewPanel::DrawToolbar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

		// 左侧返回按钮（当不是根目录时显示）
		if (m_CurrentDir != m_AssetsDir)
		{
			if (ImGui::Button("<-"))
				m_CurrentDir = m_CurrentDir.parent_path();
			ImGui::SameLine();
		}

		// 路径信息
		ImGui::TextDisabled("Path:");
		ImGui::SameLine();

		std::string displayPath = std::filesystem::absolute(m_CurrentDir).string();
		if (displayPath.empty()) displayPath = "Assets";
		ImGui::Text(displayPath.c_str());

		// 右对齐模式切换按钮
		float buttonWidth = 150.0f;
		float paddingRight = ImGui::GetStyle().WindowPadding.x;
		// 计算同一行放置按钮的位置（尽量右对齐）
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonWidth - paddingRight);
		if (ImGui::Button(m_Mode == BrowserMode::Directory ? "Mode: Directory" : "Mode: Category", ImVec2(buttonWidth, 0)))
		{
			// 切换模式
			m_Mode = (m_Mode == BrowserMode::Directory) ? BrowserMode::Category : BrowserMode::Directory;

			if (m_Mode == BrowserMode::Category)
			{
				// 进入分类时默认 Scenes，并立即填充右侧
				m_SelectedCategory = "Scenes";
				m_SelectedFile.clear();
				m_CategoryPreviewFiles = m_SceneFiles;
			}
			else
			{
				// 回到目录模式把当前目录设为根
				m_CurrentDir = m_AssetsDir;
				m_SelectedCategory.clear();
				m_CategoryPreviewFiles.clear();
			}
		}

		ImGui::PopStyleVar();
	}

	void FolderPreviewPanel::ScanAssetsForCategories(const std::filesystem::path& directory)
	{
		m_ModelFiles.clear();
		m_TextureFiles.clear();
		m_SceneFiles.clear();

		for (auto& entry : std::filesystem::recursive_directory_iterator(directory))
		{
			if (!entry.is_regular_file())
				continue;

			std::string ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if (ext == ".fbx" || ext == ".obj" || ext == ".gltf")
				m_ModelFiles.push_back(entry.path());
			else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
				m_TextureFiles.push_back(entry.path());
			else if (ext == ".scene" || ext == ".hscene")
				m_SceneFiles.push_back(entry.path());
		}
	}

	void FolderPreviewPanel::DrawCategoryTree()
	{
		struct CategoryEntry { std::string Name; std::vector<std::filesystem::path>* Files; };
		std::vector<CategoryEntry> categories = {
			{"Scenes", &m_SceneFiles},
			{"Models", &m_ModelFiles},
			{"Textures", &m_TextureFiles}
		};

		for (auto& category : categories)
		{
			ImGui::PushID(category.Name.c_str());

			bool isCategorySelected = (category.Name == m_SelectedCategory);

			// 图标 + 分类名
			ImGui::AlignTextToFramePadding();
			ImGui::Image(m_DirectoryIcon.textureID->RawHandle(), ImVec2(16, 16), ImVec2(0, 1), ImVec2(1, 0));
			ImGui::SameLine();

			if (ImGui::Selectable(category.Name.c_str(), isCategorySelected, ImGuiSelectableFlags_SpanAvailWidth))
			{
				m_SelectedCategory = category.Name;
				m_SelectedFile.clear();
				m_CategoryPreviewFiles = *category.Files; // 立即填充右侧
			}

			ImGui::PopID();
		}
	}
} // namespace GameEngine
