#include "hzpch.h"
#include "PresentPass.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
#include "Hazel/Scene/SceneManager.h"

namespace GameEngine
{ 
	void PresentPass::Init()
	{
	}

	void PresentPass::Build(RDGBuilder& builder)
	{
        RHITextureRef swapchainTexture = APP_SWAPCHAIN->GetTexture(APP_FRAMEINDEX);

        RDGTextureHandle outColor = builder.GetTexture("UI");

        RDGTextureHandle present = builder.CreateTexture("Present")
            .Import(swapchainTexture, RESOURCE_STATE_PRESENT)
            .Finish();

        RDGPresentPassHandle pass = builder.CreatePresentPass(GetName())
            .PresentTexture(present)
            .Texture(outColor)
            .Finish();
    }
}