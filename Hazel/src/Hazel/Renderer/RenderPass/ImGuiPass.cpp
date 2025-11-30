#include "hzpch.h"
#include "ImGuiPass.h"
#include "Hazel/Core/Application.h"
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
#include <Hazel/Editor/PanelManager.h>
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Scene/SceneManager.h"

namespace GameEngine
{
    void ImGuiPass::Init()
    {
        APP_DYNAMICRHI->InitImGui(APP_GLFWWINDOW);
        m_PanelManager = std::make_shared<PanelManager>();
        APP_RENDERSYSTEM->SetPanelManager(m_PanelManager);  // 因为要传递事件给他

    }
    void ImGuiPass::Build(RDGBuilder& builder)
	{
        if (IsEnabled())
        {
            if (viewportID[APP_FRAMEINDEX]) {
                ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)viewportID[APP_FRAMEINDEX]->RawHandle());
            }
            RDGTextureHandle viewport = builder.GetTexture("ViewPort");

            auto [w, h] = APP_WINDOWSIZE;

            RDGTextureHandle UI = builder.CreateTexture("UI")
                .Exetent({ w, h ,1 })
                .Format(FORMAT_R8G8B8A8_UNORM)
                .AllowRenderTarget()
                .Finish();
            
            RDGRenderPassHandle pass = builder.CreateRenderPass(GetName())
                .Color(0, UI, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f })
                .Read(0,0,0, viewport)  // 只是使用也可以这样防止不创建资源
                .Execute([&](RDGPassContext context) {
                        auto [w, h] = APP_WINDOWSIZE;
                        Extent2D windowExtent = { w, h };
                        RHICommandListRef command = context.command;
        
                        viewportID[APP_FRAMEINDEX] = Texture::GetImGuiID(builder.GetRHITexture("ViewPort"));

                        ImGui_ImplVulkan_NewFrame();
                        ImGui_ImplGlfw_NewFrame();
                        ImGui::NewFrame();
                        m_PanelManager->SetGPUTimeInfo(RENDER_GPU_TIME_INFO);
                        m_PanelManager->ImGuiCommand(viewportID[APP_FRAMEINDEX], viewportID[APP_FRAMEINDEX]);
                        ImGui::Render();
                        command->ImGuiRenderDrawData();
                    })
                .Finish();




        }

	}

}