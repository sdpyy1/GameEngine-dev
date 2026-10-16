#include "hzpch.h"
#include "SerializationManagerPanel.h"

#include <algorithm>
#include <cstdio>
#include <imgui.h>

namespace GameEngine {

	SerializationManagerPanel::SerializationManagerPanel()
	{
		Refresh();
	}

	void SerializationManagerPanel::Refresh()
	{
		m_Models = AssetManager::ScanModelSerialization();
		m_Status = "Scanned " + std::to_string(m_Models.size()) + " model source files.";
	}

	std::string SerializationManagerPanel::FormatBytes(uint64_t bytes)
	{
		constexpr double KiB = 1024.0;
		constexpr double MiB = KiB * 1024.0;
		constexpr double GiB = MiB * 1024.0;
		char buffer[64]{};
		if (bytes >= static_cast<uint64_t>(GiB))
			std::snprintf(buffer, sizeof(buffer), "%.2f GiB", bytes / GiB);
		else if (bytes >= static_cast<uint64_t>(MiB))
			std::snprintf(buffer, sizeof(buffer), "%.2f MiB", bytes / MiB);
		else if (bytes >= static_cast<uint64_t>(KiB))
			std::snprintf(buffer, sizeof(buffer), "%.2f KiB", bytes / KiB);
		else
			std::snprintf(buffer, sizeof(buffer), "%llu B", static_cast<unsigned long long>(bytes));
		return buffer;
	}

	std::string SerializationManagerPanel::DisplayPath(const std::filesystem::path& path) const
	{
		std::error_code ec;
		const std::filesystem::path relative = std::filesystem::relative(path, std::filesystem::path(APP_WORKING_DIR), ec);
		return !ec ? relative.string() : path.string();
	}

	void SerializationManagerPanel::OnImGuiRender()
	{
		if (!isOpen)
			return;

		ImGui::Begin("Serialization Manager", &isOpen);
		if (ImGui::Button("Refresh"))
			Refresh();
		ImGui::SameLine();
		ImGui::TextDisabled("Models under Assets/Model");

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Serialization Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("Build SDF", &m_SerializationSettings.buildDistanceField);
			ImGui::BeginDisabled(!m_SerializationSettings.buildDistanceField);
			int resolution = static_cast<int>(m_SerializationSettings.distanceFieldResolution);
			if (ImGui::InputInt("SDF Resolution", &resolution))
				m_SerializationSettings.distanceFieldResolution = static_cast<uint32_t>(std::clamp(resolution, 2, 256));
			int padding = static_cast<int>(m_SerializationSettings.distanceFieldPadding);
			if (ImGui::InputInt("SDF Padding (voxels)", &padding))
				m_SerializationSettings.distanceFieldPadding = static_cast<uint32_t>(std::clamp(padding, 0, 16));
			ImGui::Checkbox("Signed Distance", &m_SerializationSettings.distanceFieldSigned);
			ImGui::EndDisabled();
		}

		if (!m_Status.empty())
			ImGui::TextDisabled("%s", m_Status.c_str());
		if (m_HasLastReport)
			DrawLastSerializationReport();

		const size_t unserializedCount = static_cast<size_t>(std::count_if(m_Models.begin(), m_Models.end(), [](const auto& model) {
			return !model.IsSerialized();
		}));
		ImGui::SetNextItemOpen(false, ImGuiCond_Once);
		const bool showUnserialized = unserializedCount == 0 || ImGui::CollapsingHeader(
			("Unserialized Models (" + std::to_string(unserializedCount) + ")").c_str(), 0);

		if (ImGui::BeginTable("ModelSerializationTable", 4,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Source Model", ImGuiTableColumnFlags_WidthStretch, 0.46f);
			ImGui::TableSetupColumn("Serialized Asset", ImGuiTableColumnFlags_WidthStretch, 0.30f);
			ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 230.0f);
			ImGui::TableHeadersRow();

			for (size_t modelIndex = 0; modelIndex < m_Models.size(); ++modelIndex)
			{
				const ModelSerializationInfo& model = m_Models[modelIndex];
				const bool isSerialized = model.IsSerialized();
				if (!isSerialized && !showUnserialized)
					continue;

				ImGui::PushID(model.sourcePath.string().c_str());
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::TextWrapped("%s", DisplayPath(model.sourcePath).c_str());

				ImGui::TableSetColumnIndex(1);
				if (isSerialized)
					ImGui::TextWrapped("%s", DisplayPath(model.serializedPath).c_str());
				else
					ImGui::TextDisabled("Not generated");

				ImGui::TableSetColumnIndex(2);
				if (isSerialized)
					ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.0f), "Serialized");
				else
					ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.25f, 1.0f), "Missing");

				ImGui::TableSetColumnIndex(3);
				if (ImGui::Button(isSerialized ? "Re-serialize" : "Serialize"))
				{
					const std::filesystem::path sourcePath = model.sourcePath;
					ModelSerializationReport report;
					const bool success = AssetManager::SerializeModel(sourcePath, m_SerializationSettings, &report);
					Refresh();
					m_Status = success ? "Serialized: " + DisplayPath(sourcePath)
					              : "Serialization failed: " + DisplayPath(sourcePath);
					if (success)
					{
						m_LastReport = std::move(report);
						m_HasLastReport = true;
					}
					ImGui::PopID();
					break;
				}
				ImGui::SameLine();
				if (ImGui::BeginDisabled(!isSerialized), ImGui::Button("Delete"))
				{
					m_DeleteTarget = model.sourcePath;
					m_ShowDeleteConfirmation = true;
				}
				ImGui::EndDisabled();
				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		DrawDeleteConfirmation();
		ImGui::End();
	}

	void SerializationManagerPanel::DrawLastSerializationReport()
	{
		if (!ImGui::CollapsingHeader("Last Serialization Report", ImGuiTreeNodeFlags_DefaultOpen))
			return;

		ImGui::Text("Source file: %s", FormatBytes(m_LastReport.sourceBytes).c_str());
		ImGui::Text("Serialized payload: %s", FormatBytes(m_LastReport.serializedBytes).c_str());
		ImGui::Text("SubMeshes: %llu | Vertices: %llu | Indices: %llu | Triangles: %llu",
			static_cast<unsigned long long>(m_LastReport.submeshes.size()),
			static_cast<unsigned long long>(m_LastReport.vertexCount),
			static_cast<unsigned long long>(m_LastReport.indexCount),
			static_cast<unsigned long long>(m_LastReport.triangleCount));
		ImGui::Text("Geometry data: %s", FormatBytes(m_LastReport.geometryBytes).c_str());
		ImGui::Text("SDF build time: %.3f ms", m_LastReport.distanceFieldBuildTimeMs);
		ImGui::Text("SDF samples: %llu | Data: %s | CPU storage: %s",
			static_cast<unsigned long long>(m_LastReport.distanceFieldSampleCount),
			FormatBytes(m_LastReport.distanceFieldDataBytes).c_str(),
			FormatBytes(m_LastReport.distanceFieldStorageBytes).c_str());

		for (size_t index = 0; index < m_LastReport.submeshes.size(); ++index)
		{
			const SubmeshSerializationStats& submesh = m_LastReport.submeshes[index];
			ImGui::PushID(static_cast<int>(index));
			const std::string label = "SubMesh " + std::to_string(index) + ": " +
				(submesh.name.empty() ? "Unnamed" : submesh.name);
			if (ImGui::TreeNode(label.c_str()))
			{
				ImGui::Text("Vertices: %llu | Indices: %llu | Triangles: %llu",
					static_cast<unsigned long long>(submesh.vertexCount),
					static_cast<unsigned long long>(submesh.indexCount),
					static_cast<unsigned long long>(submesh.triangleCount));
				ImGui::Text("Geometry data: %s", FormatBytes(submesh.geometryBytes).c_str());
				if (submesh.distanceFieldBuildAttempted)
				{
					ImGui::Text("SDF: %s | Build: %.3f ms",
						submesh.distanceFieldValid ? "Valid" : "Failed",
						submesh.distanceFieldBuildTimeMs);
					ImGui::Text("Resolution: %ux%ux%u | Samples: %llu",
						submesh.distanceFieldResolutionX,
						submesh.distanceFieldResolutionY,
						submesh.distanceFieldResolutionZ,
						static_cast<unsigned long long>(submesh.distanceFieldSampleCount));
					ImGui::Text("SDF data: %s | CPU storage: %s",
						FormatBytes(submesh.distanceFieldDataBytes).c_str(),
						FormatBytes(submesh.distanceFieldStorageBytes).c_str());
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
	}

	void SerializationManagerPanel::DrawDeleteConfirmation()
	{
		if (!m_ShowDeleteConfirmation)
			return;

		ImGui::OpenPopup("Confirm serialized model deletion");
		bool open = true;
		if (ImGui::BeginPopupModal("Confirm serialized model deletion", &open, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextWrapped("Delete the serialized Model asset?\n%s", DisplayPath(AssetManager::GetModelSerializedPath(m_DeleteTarget)).c_str());
			ImGui::Separator();
			if (ImGui::Button("Delete", ImVec2(100.0f, 0.0f)))
			{
				const bool success = AssetManager::DeleteSerializedModel(m_DeleteTarget);
				m_ShowDeleteConfirmation = false;
				m_HasLastReport = false;
				ImGui::CloseCurrentPopup();
				Refresh();
				m_Status = success ? "Deleted serialized asset." : "Serialized asset was not deleted.";
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(100.0f, 0.0f)))
			{
				m_ShowDeleteConfirmation = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (!open)
			m_ShowDeleteConfirmation = false;
	}
}
