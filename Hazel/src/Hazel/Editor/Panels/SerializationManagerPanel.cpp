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
		m_ModelStats.clear();
		m_ModelStats.reserve(m_Models.size());
		m_ModelStats.resize(m_Models.size());
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

		if (!m_Status.empty())
			ImGui::TextDisabled("%s", m_Status.c_str());

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
				const ModelMemoryStats& stats = m_ModelStats[modelIndex];
				ImGui::PushID(model.sourcePath.string().c_str());
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth;
				const bool detailsOpen = ImGui::TreeNodeEx("##model_details", treeFlags, "%s", DisplayPath(model.sourcePath).c_str());
				if (detailsOpen)
				{
					DrawModelDetails(model);
					ImGui::TreePop();
				}

				ImGui::TableSetColumnIndex(1);
				if (model.IsSerialized())
					ImGui::TextWrapped("%s", DisplayPath(model.serializedPath).c_str());
				else
					ImGui::TextDisabled("Not generated");

				ImGui::TableSetColumnIndex(2);
				if (stats.inspected && !stats.valid)
					ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Unsupported");
				else if (model.IsSerialized())
					ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.0f), "Serialized");
				else
					ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.25f, 1.0f), "Missing");

				ImGui::TableSetColumnIndex(3);
				const bool canSerialize = !model.IsSerialized() || !stats.inspected || stats.valid;
				ImGui::BeginDisabled(!canSerialize);
				if (ImGui::Button(model.IsSerialized() ? "Re-serialize" : "Serialize"))
				{
					const std::filesystem::path sourcePath = model.sourcePath;
					const bool success = AssetManager::SerializeModel(sourcePath);
					Refresh();
					m_Status = success ? "Serialized: " + DisplayPath(sourcePath)
					              : "Serialization failed: " + DisplayPath(sourcePath);
					ImGui::EndDisabled();
					ImGui::PopID();
					break;
				}
				ImGui::EndDisabled();
				ImGui::SameLine();
				if (ImGui::BeginDisabled(!model.IsSerialized()), ImGui::Button("Delete"))
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

	void SerializationManagerPanel::DrawModelDetails(const ModelSerializationInfo& model)
	{
		const auto modelIt = std::find_if(m_Models.begin(), m_Models.end(), [&model](const auto& candidate) {
			return candidate.sourcePath == model.sourcePath;
		});
		if (modelIt == m_Models.end())
			return;
		const size_t modelIndex = static_cast<size_t>(std::distance(m_Models.begin(), modelIt));
		ModelMemoryStats& stats = m_ModelStats[modelIndex];

		ImGui::Indent(18.0f);
		if (!model.IsSerialized())
		{
			ImGui::TextDisabled("No serialized asset. Source statistics are not loaded.");
			ImGui::Unindent(18.0f);
			return;
		}
		if (!stats.inspected)
			stats = AssetManager::InspectModel(model.sourcePath);
		if (!stats.valid)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Inspection failed: %s", stats.error.c_str());
			ImGui::Unindent(18.0f);
			return;
		}

		ImGui::Text("Source file: %s", FormatBytes(stats.sourceBytes).c_str());
		if (model.IsSerialized())
		{
			std::error_code ec;
			const uintmax_t serializedBytes = std::filesystem::file_size(model.serializedPath, ec);
			if (!ec)
				ImGui::Text("Serialized payload: %s", FormatBytes(static_cast<uint64_t>(serializedBytes)).c_str());
		}
		ImGui::Text("CPU memory estimate: %s", FormatBytes(stats.totalBytes).c_str());
		ImGui::Text("SubMeshes: %llu | Vertices: %llu | Indices: %llu | Triangles: %llu",
			static_cast<unsigned long long>(stats.submeshes.size()),
			static_cast<unsigned long long>(stats.vertexCount),
			static_cast<unsigned long long>(stats.indexCount),
			static_cast<unsigned long long>(stats.triangleCount));

		for (size_t index = 0; index < stats.submeshes.size(); ++index)
		{
			const MeshMemoryStats& mesh = stats.submeshes[index];
			ImGui::PushID(static_cast<int>(index));
			if (ImGui::TreeNode("##submesh_details", "SubMesh %zu: %s", index, mesh.name.empty() ? "Unnamed" : mesh.name.c_str()))
			{
				ImGui::Text("Memory: %s (object %s + arrays %s)", FormatBytes(mesh.TotalBytes()).c_str(),
					FormatBytes(mesh.objectBytes).c_str(), FormatBytes(mesh.GeometryBytes()).c_str());
				ImGui::Text("Vertices: %llu | Indices: %llu | Triangles: %llu",
					static_cast<unsigned long long>(mesh.vertexCount),
					static_cast<unsigned long long>(mesh.indexCount),
					static_cast<unsigned long long>(mesh.triangleCount));
				ImGui::Text("Position %s | Normal %s | Tangent %s | UV %s",
					FormatBytes(mesh.positionBytes).c_str(), FormatBytes(mesh.normalBytes).c_str(),
					FormatBytes(mesh.tangentBytes).c_str(), FormatBytes(mesh.texCoordBytes).c_str());
				ImGui::Text("Color %s | Bone index %s | Bone weight %s | Index %s | Bone data %s",
					FormatBytes(mesh.colorBytes).c_str(), FormatBytes(mesh.boneIndexBytes).c_str(),
					FormatBytes(mesh.boneWeightBytes).c_str(), FormatBytes(mesh.indexBytes).c_str(),
					FormatBytes(mesh.boneBytes).c_str());
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::Unindent(18.0f);
	}

	void SerializationManagerPanel::DrawDeleteConfirmation()
	{
		if (!m_ShowDeleteConfirmation)
			return;

		// Open and render the popup at the same ID stack level. The delete button
		// is inside a per-model PushID scope, so opening it there would create a
		// different popup ID from this function.
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
