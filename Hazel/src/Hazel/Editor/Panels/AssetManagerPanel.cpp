#include "hzpch.h"

#include "AssetManagerPanel.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/SceneManager.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <filesystem>
#include "Hazel/Asset/AssetManager.h"
#ifdef _MSVC_LANG
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace GameEngine {
	AssetManagerPanel::AssetManagerPanel()
	{
		m_EntityIcon.LoadIconData("Assets/Icon/Entity.png", false);
		m_DirLightIcon.LoadIconData("Assets/Icon/Sun.png", false);
		m_SpotLightIcon.LoadIconData("Assets/Icon/Spotlight.png", false);
		m_PointLightIcon.LoadIconData("Assets/Icon/pointLight.png", false);
		m_SkyLightIcon.LoadIconData("Assets/Icon/img.png", false);
		m_PostprocesstIcon.LoadIconData("Assets/Icon/post.png", false);
		m_ProbeIcon.LoadIconData("Assets/Icon/probe.png", false);
		m_CameraIcon.LoadIconData("Assets/Icon/camera.png", false);
	}

	void AssetManagerPanel::DrawComponents(Entity entity)
	{
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
			DisplayAddComponentEntry<ModelComponent>("Model");
			DisplayAddComponentEntry<SubmeshComponent>("Submesh");
			DisplayAddComponentEntry<DirectionalLightComponent>("DirctionalLight");
			DisplayAddComponentEntry<PostProcessingComponent>("PostProcessing");
			DisplayAddComponentEntry<SpotLightComponent>("SpotLight");
			DisplayAddComponentEntry<LightProbeComponent>("Light Probe");
			DisplayAddComponentEntry<CameraComponent>("Camera");
			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
			{
				DrawVec3Control("Translation", component.Translation);

				glm::vec3 rotationEuler = component.GetRotationEuler();
				glm::vec3 rotation = glm::degrees(rotationEuler);
				DrawVec3Control("Rotation", rotation);
				component.SetRotationEuler(glm::radians(rotation));

				// static bool lockScale = true;
				// ImGui::Checkbox("Lock Scale", &lockScale);  // TODO：锁定缩放有Bug，会让缩放突然变化
				DrawVec3Control("Scale", component.Scale, 1.0f);
			});

		DrawComponent<SubmeshComponent>("Submesh", entity, [](auto& component)
			{
				ImGui::Checkbox("Visible", &component.Visible);

				MaterialRef material = component.GetMaterial();

				DrawMaterialVec4("Diffuse Color", material->diffuse, [&](const glm::vec4& val) { material->SetDiffuse(val); });
				DrawMaterialVec4("Emission Color", material->emission, [&](const glm::vec4& val) { material->SetEmission(val); });
				DrawMaterialFloat("Roughness", material->roughness, [&](float val) { material->SetRoughness(val); });
				DrawMaterialFloat("Metallic", material->metallic, [&](float val) { material->SetMetallic(val); });

				DrawTextureSlot("Diffuse", material->textureDiffuse, [&](TextureRef tex) {material->SetDiffuse(tex); });
				DrawTextureSlot("Normal", material->textureNormal, [&](TextureRef tex) {material->SetNormal(tex); });
				DrawTextureSlot("Roughness", material->textureRoughness, [&](TextureRef tex) {material->SetRoughness(tex); });
				DrawTextureSlot("Metallic", material->textureMetallic, [&](TextureRef tex) {material->SetMetallic(tex); });
				DrawTextureSlot("Emission", material->textureEmission, [&](TextureRef tex) {material->SetEmission(tex); });
			});

		DrawComponent<ModelComponent>("Model", entity, [](auto& component)
			{
				ImGui::Checkbox("Visible", &component.Visible);
				ImGui::Checkbox("Cast Shadow", &component.castShadow);
			});

		DrawComponent<DirectionalLightComponent>("DirectionalLight", entity, [](auto& component)
			{
				ImGui::ColorEdit3("Radiance", &component.Radiance.x, ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 100.0f, "%.2f");
				
				ImGui::Checkbox("Show Direction", &component.showDirection);
				ImGui::Checkbox("CSM Smooth", &component.CSMSmooth);
				ImGui::Checkbox("Show CSM", &component.showCSM);
			});
		DrawComponent<PointLightComponent>("Point Light", entity, [](auto& component)
			{
				ImGui::ColorEdit3("Radiance", &component.Radiance.x, ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 3.0f, "%.2f");
				ImGui::DragFloat("Radius", &component.Radius, 0.1f, 0.0f, 50.0f, "%.2f");
				ImGui::Checkbox("Show Radius", &component.showRadius);
			});
		DrawComponent<SpotLightComponent>("Spot Light", entity, [](auto& component)
			{
				ImGui::ColorEdit3("Radiance", &component.Radiance.x, ImGuiColorEditFlags_Float);
				ImGui::DragFloat("Intensity", &component.Intensity, 0.1f, 0.0f, 10.0f, "%.2f");
				ImGui::DragFloat("Range", &component.range, 0.1f, 0.0f, 100.0f, "%.2f");
				ImGui::DragFloat("Angle", &component.angle, 0.1f, 1.0f, 179.0f, "%.1f");
				ImGui::DragFloat("Falloff", &component.falloff, 0.01f, 0.0f, 10.0f, "%.2f");
				ImGui::Checkbox("Show Radius", &component.showRadius);
				ImGui::Checkbox("Show Direction", &component.showDirection);
			});
		DrawComponent<PostProcessingComponent>("PostProcess", entity, [](auto& component)
			{
				// ===================== Bloom =====================
				if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Enable Bloom", &component.enableBloom);
					ImGui::BeginDisabled(!component.enableBloom);
					{
						ImGui::SliderFloat("Bloom Intensity",&component.bloomScale,0.0f, 2.0f,"%.2f");
					}
					ImGui::EndDisabled();
				}

				// ===================== TAA =====================
				if (ImGui::CollapsingHeader("TAA", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Enable TAA", &component.enableTAA);

					ImGui::BeginDisabled(!component.enableTAA);
					{
						ImGui::Checkbox("Sharpen", &component.taaSharpen);
						ImGui::BeginDisabled(!component.taaSharpen);
						{
							ImGui::SliderFloat("Sharpen Strength",&component.taaSharpness,0.0f, 2.0f,"%.2f");
						}
						ImGui::EndDisabled();
					}
					ImGui::EndDisabled();
				}

                // ===================== FXAA =====================
				if (ImGui::CollapsingHeader("FXAA", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Enable FXAA", &component.enableFXAA);

					ImGui::BeginDisabled(!component.enableFXAA);
					{
						ImGui::Checkbox("show Edge", &component.showEdge);
					}
					ImGui::EndDisabled();
				}

				// ===================== Path Tracing =====================
				if (ImGui::CollapsingHeader("Path Tracing", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Checkbox("Enable Path Tracing", &component.pathTracingEnable);

					ImGui::BeginDisabled(!component.pathTracingEnable);
					{
						ImGui::SliderInt("Samples Per Frame",&component.pathTracingNumSamples,1, 16);
						ImGui::SliderInt("Max Bounces",&component.pathTracingNumBounce,1, 10);
						ImGui::Checkbox("Sample Skybox",&component.pathTracingSampleSkyBox);
						ImGui::Checkbox("Indirect Only",&component.pathTracingIndirectOnly);
						ImGui::Checkbox("Active History",&component.pathTracingHistoryActive);
					}
					ImGui::EndDisabled();
				}

				// ===================== Color Grading =====================
				if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::SliderFloat("Exposure",&component.exposure,0.01f, 5.0f,"%.2f");
					ImGui::SliderFloat("Saturation",&component.saturation,0.0f, 2.0f,"%.2f");
					ImGui::SliderFloat("Contrast",&component.contrast,0.5f, 2.0f,"%.2f");
					static const char* toneMappingItems[] = {
						"CryEngine",
						"Uncharted",
						"ACES",
						"None"
					};
					ImGui::Combo("Tone Mapping",reinterpret_cast<int*>(&component.toneMappingMode),toneMappingItems,IM_ARRAYSIZE(toneMappingItems));
				}
			});

		DrawComponent<LightProbeComponent>("Light Probes", entity, [](auto& component)
			{
				ImGui::Checkbox("Enable", &component.enable);

				ImGui::BeginDisabled(!component.enable);
				{
					ImGui::SliderInt3("Probe Count", reinterpret_cast<int*>(&component.probeCount), 1, 32);
					ImGui::SliderFloat3("Grid Step", reinterpret_cast<float*>(&component.gridStep), 0.1f, 10.0f);
					ImGui::SliderInt("Rays Per Probe", reinterpret_cast<int*>(&component.raysPerProbe), 1, 1024);
					ImGui::Checkbox("InfiniteBounds", &component.infiniteBounds);
					ImGui::Checkbox("GetSkyLight", &component.getSkyLight);
					ImGui::Checkbox("Visualize", &component.visulaize);
				}
				ImGui::EndDisabled();
			});
		DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
			{
				ImGui::Checkbox("Primary", &component.Primary);
			});


		DrawComponent<SkyComponent>("Sky Light", entity, [](auto& component)
			{
				ImGui::Checkbox("DynamicSky", &component.DynamicSky);
				if (component.selectedIBL > component.iblPath.size()) {
					component.selectedIBL = -1;
				}
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
				// IBLScale
				ImGui::SliderFloat("IBL Scale", &component.IBLScale, 0.0f, 1.0f);
			});

	}

	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Manager");
		std::shared_ptr<Scene> m_Context = Application::GetSceneManager()->GetActiveScene();
		if (m_Context)
		{
			// 只绘制根实体（没有父节点的实体）
			m_Context->GetRegistry().each([&](auto entityID)
				{
					Entity entity{ entityID , m_Context.get() };
					if(entity){
						// 检查是否是根实体（ParentHandle为0）
						if (!entity.HasComponent<RelationshipComponent>() ||
							entity.GetComponent<RelationshipComponent>().ParentHandle == 0)
						{
							DrawEntityNode(entity);
						}
					}

				});

			/*if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity(Entity());*/

			if (ImGui::BeginPopupContextWindow("SceneManagerContext", 1, false))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
					m_Context->CreateEntity("Empty Entity");
				if (ImGui::MenuItem("Create Directional Light"))
					m_Context->CreateEntity("Directional Light").AddComponent<DirectionalLightComponent>();
				if (ImGui::MenuItem("Create Spot Light"))
					m_Context->CreateEntity("Spot Light").AddComponent<SpotLightComponent>();
				if (ImGui::MenuItem("Create Point Light"))
					m_Context->CreateEntity("Point Light").AddComponent<PointLightComponent>();
				if (ImGui::MenuItem("Create Sky Light"))
					m_Context->CreateEntity("Sky Light").AddComponent<SkyComponent>();
				if (ImGui::MenuItem("Create PostProcess"))
					m_Context->CreateEntity("PostProcess").AddComponent<PostProcessingComponent>();
				if (ImGui::MenuItem("Create Light Probe"))
					m_Context->CreateEntity("Light Probe").AddComponent<LightProbeComponent>();
				if (ImGui::MenuItem("Create Camera"))
					m_Context->CreateEntity("Camera").AddComponent<CameraComponent>();
				ImGui::EndPopup();
			}
		}
		ImGui::End();

		// Properties panel
		ImGui::Begin("Properties");
		if (Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity())
			DrawComponents(Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity());
		ImGui::End();
	}
	void AssetManagerPanel::SetSelectedEntity(Entity entity)
	{
		Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity(entity);
	}
	void AssetManagerPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		auto& relationship = entity.GetComponent<RelationshipComponent>();
		bool hasChildren = !relationship.Children.empty();

		// 调整标志位：增加NoTreePushOnOpen，避免自动推送节点
		ImGuiTreeNodeFlags flags = ((Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity() == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
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

		IconData icon = m_EntityIcon;
		if (entity.HasComponent<DirectionalLightComponent>())
			icon = m_DirLightIcon;
		if (entity.HasComponent<SpotLightComponent>())
			icon = m_SpotLightIcon;
		if (entity.HasComponent<SkyComponent>())
			icon = m_SkyLightIcon;
		if (entity.HasComponent<PointLightComponent>())
			icon = m_PointLightIcon;
		if (entity.HasComponent<PostProcessingComponent>())
			icon = m_PostprocesstIcon;
        if (entity.HasComponent<LightProbeComponent>())
			icon = m_ProbeIcon;
        if (entity.HasComponent<CameraComponent>())
			icon = m_CameraIcon;
		ImGui::Image(icon.textureID->RawHandle(), { iconSize, iconSize });

		ImGui::SameLine(0.0f, iconSpacing);

		if (ImGui::Selectable(tag.c_str(), Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity() == entity, ImGuiSelectableFlags_None))
			Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity(entity);

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem(("EntityPopup_" + std::to_string((uint64_t)(uint32_t)entity)).c_str()))
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;
			ImGui::EndPopup();
		}

		if (opened)
		{
			ImGui::TreePush((void*)(uint64_t)(uint32_t)entity);
			for (const UUID& childId : relationship.Children)
			{
				Entity child = Application::GetSceneManager()->GetActiveScene()->GetEntityByUUID(childId);
				if (child)
					DrawEntityNode(child);
			}
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			Application::GetSceneManager()->GetActiveScene()->DestroyEntity(entity);
			if (Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity() == entity)
				Application::GetSceneManager()->GetActiveScene()->SetSelectedEntity({});
		}

		ImGui::PopID();
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, bool lock = false, float columnWidth = 100.0f)
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
		if (lock) ImGui::BeginDisabled();

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
		if (lock) ImGui::EndDisabled(); // 结束禁用块

		if (lock) {
			values.y = values.x;
			values.z = values.x;
		}
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

	template<typename T>
	void AssetManagerPanel::DisplayAddComponentEntry(const std::string& entryName)
	{
		if (!Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity().HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				Application::GetSceneManager()->GetActiveScene()->GetSelectedEntity().AddComponent<T>();
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

	static void DrawMaterialFloat(const char* label, float& value, std::function<void(float)> onChange)
	{
		ImGui::PushID(label);
		if (ImGui::DragFloat(label, &value, 0.01f, 0.0f, 2.0f))
		{
			onChange(value);
		}
		ImGui::PopID();
	}

	static void DrawMaterialInt(const char* label, int32_t& value, std::function<void(int32_t)> onChange)
	{
		ImGui::PushID(label);
		if (ImGui::DragInt(label, &value, 1))
		{
			onChange(value);
		}
		ImGui::PopID();
	}

	static void DrawMaterialVec3(const char* label, glm::vec3& value, std::function<void(const glm::vec3&)> onChange)
	{
		ImGui::PushID(label);
		if (ImGui::DragFloat3(label, glm::value_ptr(value), 0.01f))
		{
			onChange(value);
		}
		ImGui::PopID();
	}

	static void DrawMaterialVec4(const char* label, glm::vec4& value, std::function<void(const glm::vec4&)> onChange)
	{
		ImGui::PushID(label);
		if (ImGui::ColorEdit4(label, glm::value_ptr(value)))
		{
			onChange(value);
		}
		ImGui::PopID();
	}

	static void DrawTextureSlot(const char* label, TextureRef& texture, std::function<void(TextureRef)> onDrop)
	{
		ImGui::Text("%s", label);

		ImGui::PushID(label);
		ImVec2 size = ImVec2(64, 64);

		if (texture)
		{
			RHIDescriptorSetRef texID = texture->GetImGuiID();
			ImGui::Image(texID->RawHandle(), size);
		}
		else
		{
			ImGui::Button("No Texture", size);
		}

		// 拖拽替换纹理
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				IM_ASSERT(payload->DataSize > 0);
				const char* droppedPath = (const char*)payload->Data;
				TextureSpec spec;
				spec.path = droppedPath;

				TextureRef droppedTexture = std::make_shared<Texture>(spec);
				if (droppedTexture)
				{
					onDrop(droppedTexture);
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();
	}
} // namespace GameEngine