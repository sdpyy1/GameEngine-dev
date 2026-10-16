#pragma once

#include "EditorPanel.h"
#include "Hazel/Asset/AssetManager.h"

namespace GameEngine {
	class SerializationManagerPanel : public EditorPanel
	{
	public:
		SerializationManagerPanel();
		void OnImGuiRender() override;

	private:
		void Refresh();
		void DrawDeleteConfirmation();
		void DrawLastSerializationReport();
		std::string DisplayPath(const std::filesystem::path& path) const;
		static std::string FormatBytes(uint64_t bytes);

	private:
		std::vector<ModelSerializationInfo> m_Models;
		ModelSpec m_SerializationSettings;
		ModelSerializationReport m_LastReport;
		std::filesystem::path m_DeleteTarget;
		bool m_ShowDeleteConfirmation = false;
		bool m_HasLastReport = false;
		std::string m_Status;
	};
}
