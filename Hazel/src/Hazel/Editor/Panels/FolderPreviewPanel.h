#pragma once
#include <filesystem>
#include <imgui.h>
#include "Hazel/Core/Base.h"
#include "Hazel/Asset/AssetManager.h"
#include "EditorPanel.h"
#include "Hazel/Scene/Scene.h"
enum class BrowserMode
{
	Directory,
	Category
};
namespace GameEngine {
	class FolderPreviewPanel : public EditorPanel
	{
	public:
		FolderPreviewPanel(const std::filesystem::path& assetsDir);

		void OnImGuiRender() override;
		void SetContext(std::shared_ptr<Scene>& context);
		bool createModel = false;

	private:
		void DrawToolbar();
		void OnFileOpen(const std::filesystem::path& path);
		void DrawDirectoryTree(const std::filesystem::path& directory);
		void DrawPreviewWindow();
		void DrawFileGrid();
		void ScanAssetsForCategories(const std::filesystem::path& directory);
		void DrawCategoryTree();


	private:
		enum class BrowserMode
		{
			Directory,
			Category
		};

		BrowserMode m_Mode = BrowserMode::Category;
		std::filesystem::path m_AssetsDir;
		std::filesystem::path m_CurrentDir;
		std::shared_ptr<Scene> m_Context;
		std::unordered_set<std::string> m_ExpandedFolders;

		IconData m_DirectoryIcon;
		IconData m_FileIcon;
		IconData m_SceneIcon;
		IconData m_ModelIcon;
		IconData m_PreviewTexture;

		std::filesystem::path m_SelectedFile;
		std::string m_SelectedCategory;
		std::vector<std::filesystem::path> m_ModelFiles;
		std::vector<std::filesystem::path> m_TextureFiles;
		std::vector<std::filesystem::path> m_SceneFiles;
		std::vector<std::filesystem::path> m_CategoryPreviewFiles;

		bool m_ShowPreview = false;
		std::filesystem::path m_PreviewPath;
	};
}
