#include "hzpch.h"
#include "ImGuiPass.h"
#include "Hazel/Core/Application.h"
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
#include <Hazel/Editor/ImGuiRendererManager.h>
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"

namespace GameEngine
{
    void ImGuiPass::Init()
    {
        APP_DYNAMICRHI->InitImGui(APP_GLFWWINDOW);
        m_ImGuiRendererManager = std::make_shared<ImGuiRendererManager>();
    }
    void ImGuiPass::Build(RDGBuilder& builder)
	{
        if (IsEnabled())
        {
            RDGTextureHandle viewport = builder.GetTexture("ViewPort");

            auto [w, h] = APP_WINDOWSIZE;

            RDGTextureHandle UI = builder.CreateTexture("UI")
                .Exetent({ w, h ,1 })
                .Format(FORMAT_R8G8B8A8_UNORM)
                .AllowRenderTarget()
                .Finish();


            RDGRenderPassHandle pass = builder.CreateRenderPass(GetName())
                .Color(0, UI, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f })
                .Read(0,0,1, builder.GetTexture("ViewPort"))  // 只是使用也可以这样防止不创建资源
                .Execute([&](RDGPassContext context) {
                        auto [w, h] = APP_WINDOWSIZE;
                        Extent2D windowExtent = { w, h };

                        RHICommandListRef command = context.command;
                        command->SetViewport({ 0, 0 }, { windowExtent.width, windowExtent.height });
                        command->SetScissor({ 0, 0 }, { windowExtent.width, windowExtent.height });
                        
                        ImGui_ImplVulkan_NewFrame();
                        ImGui_ImplGlfw_NewFrame();
                        ImGui::NewFrame();
                        static RHIDescriptorSetRef descriptor = V2::Texture::GetImGuiID(builder.GetRHITexture("ViewPort"));
                        m_ImGuiRendererManager->SetGPUTimeInfo(RENDER_GPU_TIME_INFO);
                        m_ImGuiRendererManager->ImGuiCommand(descriptor);
                        ImGui::Render();
                        command->ImGuiRenderDrawData();
                    })
                .Finish();




        }

	}

}