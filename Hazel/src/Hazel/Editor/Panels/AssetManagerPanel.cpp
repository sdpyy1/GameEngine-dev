#include "hzpch.h"

#include "AssetManagerPanel.h"
#include "Hazel/Scene/Components.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <filesystem>
#include <Hazel/Utils/UIUtils.h>
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Asset/Model/Mesh.h"
#ifdef _MSVC_LANG
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace GameEngine {
	AssetManagerPanel::AssetManagerPanel()
	{
		m_EntityIcon.LoadIconData("Assets/Icon/Entity.png",false);
		m_DirLightIcon.LoadIconData("Assets/Icon/Sun.png", false);
		m_SpotLightIcon.LoadIconData("Assets/Icon/Spotlight.png", false);
		m_SkyLightIcon.LoadIconData("Assets/Icon/img.png", false);
	}

	void AssetManagerPanel::SetContext(std::shared_ptr<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}

	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Manager");

		if (m_Context)
		{
			// 只绘制根实体（没有父节点的实体）
			m_Context->GetRegistry().each([&](auto entityID)
				{
					Entity entity{ entityID , m_Context.get()};
					// 检查是否是根实体（ParentHandle为0）
					if (!entity.HasComponent<RelationshipComponent>() ||
						entity.GetComponent<RelationshipComponent>().ParentHandle == 0)
					{
						DrawEntityNode(entity);
					}
				});

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			if (ImGui::BeginPopupContextWindow("SceneManagerContext", 1, false))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
					m_Context->CreateEntity("Empty Entity");
				if (ImGui::MenuItem("Create Directional Light"))
					m_Context->CreateEntity("Directional Light").AddComponent<DirectionalLightComponent>();
				if (ImGui::MenuItem("Create Spot Light"))
					m_Context->CreateEntity("Spot Light").AddComponent<SpotLightComponent>();
				if (ImGui::MenuItem("Create Sky Light"))
					m_Context->CreateEntity("Sky Light").AddComponent<SkyComponent>(); // TODO: add component
				ImGui::EndPopup();
			}
		}
		ImGui::End();

		// Properties panel
		ImGui::Begin("Properties");
		if (m_SelectionContext)
			DrawComponents(m_SelectionContext);
		ImGui::End();
	}
	void AssetManagerPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}
	void AssetManagerPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		auto& relationship = entity.GetComponent<RelationshipComponent>();
		bool hasChildren = !relationship.Children.empty();

		// 调整标志位：增加NoTreePushOnOpen，避免自动推送节点
		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_AllowItemOverlap |
			ImGuiTreeNodeFlags_NoTreePushOnOpen;  // 关键：不自动推送树节点

		if (!hasChildren)
			flags |= ImGuiTreeNodeFlags_Leaf;

		float iconSize = 16.0f;
		float iconSpacing = 5.0f;

		ImGui::PushID((void*)(uint64_t)(uint32_t)entity);

		// 绘制箭头（TreeNodeEx），获取展开状态
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "");

		ImGui::SameLine(0.0f, iconSpacing);

		// 绘制图标（保持不变）
		IconData icon = m_EntityIcon;
		if (entity.HasComponent<DirectionalLightComponent>())
			icon = m_DirLightIcon;
		if (entity.HasComponent<SpotLightComponent>())
			icon = m_SpotLightIcon;
		if (entity.HasComponent<SkyComponent>())
			icon = m_SkyLightIcon;
		ImGui::Image(icon.textureID->RawHandle(), { iconSize, iconSize });


		ImGui::SameLine(0.0f, iconSpacing);

		// 调整Selectable标志位：不跨列，避免覆盖箭头
		if (ImGui::Selectable(tag.c_str(), m_SelectionContext == entity, ImGuiSelectableFlags_None))
			m_SelectionContext = entity;

		// 右键菜单（保持不变）
		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem(("EntityPopup_" + std::to_string((uint64_t)(uint32_t)entity)).c_str()))
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;
			ImGui::EndPopup();
		}

		// 递归绘制子实体（手动处理TreePush/TreePop，因为前面用了NoTreePushOnOpen）
		if (opened)
		{
			ImGui::TreePush((void*)(uint64_t)(uint32_t)entity);  // 手动推送
			for (const UUID& childId : relationship.Children)
			{
				Entity child = m_Context->GetEntityByUUID(childId);
				if (child)
					DrawEntityNode(child);
			}
			ImGui::TreePop();
		}

		// 删除实体（保持不变）
		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}

		ImGui::PopID();
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		// X
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Y
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Z
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		ImGui::Columns(1);
		ImGui::PopID();
	}

	template<typename T, typename UIFunction>
	void AssetManagerPanel::DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
			ImGuiTreeNodeFlags_FramePadding;

		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar();
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
				ImGui::OpenPopup("ComponentSettings");

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;
				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent) {
				entity.RemoveComponent<T>();
			}
		}
	}

	void AssetManagerPanel::DrawComponents(Entity entity)
	{
		// Tag
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;
			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		// Add Component
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			// TODO:这里添加新组件的添加按钮
			DisplayAddComponentEntry<ModelComponent>("StaticModel");
			DisplayAddComponentEntry<DynamicModelComponent>("DynamicModel");
			DisplayAddComponentEntry<DirectionalLightComponent>("DirctionalLight");
			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		// 这里描述组件渲染
		DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
			{
				DrawVec3Control("Translation", component.Translation);
				glm::vec3 rotationEuler = component.GetRotationEuler();
				glm::vec3 rotation = glm::degrees(rotationEuler);
				DrawVec3Control("Rotation", rotation);
				component.SetRotationEuler(glm::radians(rotation));
				DrawVec3Control("Scale", component.Scale, 1.0f);
			});
		DrawComponent<SubmeshComponent>("SubmeshComponent", entity, [](auto& component)
			{
				ImGui::Checkbox("Visible", &component.Visible);
				
			});
		DrawComponent<ModelComponent>("StaticModel", entity, [](auto& component)
			{
				ImGui::Checkbox("Visible", &component.Visible);
			});
		DrawComponent<DynamicModelComponent>("DynamicModel", entity, [](auto& component)
			{
				ImGui::Checkbox("Visible", &component.Visible);

			});


		DrawComponent<DirectionalLightComponent>("DirectionalLight", entity, [](auto& component)
			{
				ImGui::Text("DirectionalLight Add!");
			});



		DrawComponent<SpotLightComponent>("Spot Light", entity, [](auto& component)
			{
				ImGui::Text("SpotLight Add!");
				return;
			});
		DrawComponent<SkyComponent>("Sky Light", entity, [](auto& component)
			{
				ImGui::Checkbox("DynamicSky", &component.DynamicSky);
				if (component.selectedIBL > component.iblPath.size()) {
					component.selectedIBL = -1;
				}
				ImGui::SliderFloat("Bloom Scale", &component.bloomScale, 0.0f, 3.0f);
				if (!component.iblPath.empty())
				{
					std::vector<std::string> itemStrings;
					std::vector<const char*> items;

					for (auto& path : component.iblPath)
					{
						std::filesystem::path relative = std::filesystem::relative(path, "Assets");
						itemStrings.push_back(relative.u8string());
					}

					for (auto& str : itemStrings)
						items.push_back(str.c_str());

					ImGui::Text("Select HDR:");
					ImGui::Combo("##HDRCombo", &component.selectedIBL, items.data(), (int)items.size());
				}
				else
				{
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "No HDR found in Assets!");
					component.selectedIBL = -1;
				}
			});
		DrawComponent<AnimationComponent>("Animation", entity, [](auto& component)
			{
				ImGui::Text("Mesh Source Handle: %llu", (uint64_t)component.meshSource);

				Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(component.meshSource);
				if (meshSource)
				{
					std::vector<std::string> animNames = meshSource->GetAnimationNames();
					if (!animNames.empty())
					{
						component.SelectedAnimIndex = glm::clamp(component.SelectedAnimIndex, 0, (int)animNames.size() - 1);

						ImGui::Text("Animation");
						ImGui::SameLine();
						if (ImGui::BeginCombo("##AnimationSelector", animNames[component.SelectedAnimIndex].c_str()))
						{
							for (int i = 0; i < animNames.size(); i++)
							{
								bool isSelected = (component.SelectedAnimIndex == i);
								if (ImGui::Selectable(animNames[i].c_str(), isSelected))
								{
									component.SelectedAnimIndex = i;
									component.CurrentAnimation = meshSource->GetAnimation(
										animNames[i], *meshSource->GetSkeleton(), false, glm::vec3(1), 0
									);
								}
								if (isSelected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}
					}
					else
					{
						ImGui::Text("No animations available");
					}
				}
				else
				{
					ImGui::Text("Invalid MeshSource asset");
				}
			});
	}
	template<typename T>
	void AssetManagerPanel::DisplayAddComponentEntry(const std::string& entryName)
	{
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}
	bool IsImageFile(const std::string& filepath)
	{
		// 找最后一个 '.' 位置
		size_t dotPos = filepath.find_last_of('.');
		if (dotPos == std::string::npos)
			return false;

		std::string ext = filepath.substr(dotPos + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		return ext == "png" || ext == "jpg" || ext == "jpeg" ||
			ext == "tga" || ext == "bmp" || ext == "hdr";
	}
	void AssetManagerPanel::DrawMaterial(AssetHandle meshSourceHandle)
	{
		
	}
} // namespace GameEngine
