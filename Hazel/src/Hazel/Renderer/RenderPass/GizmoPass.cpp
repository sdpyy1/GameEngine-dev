#include "hzpch.h"
#include "GizmoPass.h"
#include "Hazel/Renderer/RenderResource/Shader.h"
#include "Hazel/Renderer/RenderSystem/RenderManager.h"
#include "Hazel/Renderer/RenderResource/RenderResourceManager.h"
#include "Hazel/Renderer/RenderResource/PipelineCache.h"
#include "Hazel/Scene/SceneManager.h"
void GameEngine::GizmoPass::Init()
{
	// init
	{
		m_GizmoInitShader = std::make_shared<Shader>("gizmo/GizmoInit", SHADER_FREQUENCY_COMPUTE)->GetRHIShader();
		RHIRootSignatureInfo info = {};
		info.AddEntry(RENDER_RESOURCEMANAGER->GetGlobalResourcePreFrameRootSignature()->GetInfo());
		m_RootSignature = APP_DYNAMICRHI->CreateRootSignature(info);
		RHIComputePipelineInfo pipelineInfo = {};
		pipelineInfo.computeShader = m_GizmoInitShader;
		pipelineInfo.rootSignature = m_RootSignature;
		m_GizmoInitPipeline = APP_DYNAMICRHI->CreateComputePipeline(pipelineInfo);
	}
	// Box
	{
		m_BoxVertShader = std::make_shared<Shader>("gizmo/GizmoBox", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
        m_BoxFragShader = std::make_shared<Shader>("gizmo/GizmoBox", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
		RHIGraphicsPipelineInfo pipelineInfo = {};
        pipelineInfo.vertexShader = m_BoxVertShader;
        pipelineInfo.fragmentShader = m_BoxFragShader;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
		pipelineInfo.rasterizerState = { FILL_MODE_WIREFRAME, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
		pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
		pipelineInfo.rootSignature = m_RootSignature;
		pipelineInfo.blendState.renderTargets[0].enable = false;
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		VertexElement vertexElement = {};
		vertexElement.streamIndex = 0;
		vertexElement.attributeIndex = 0;
		vertexElement.format = FORMAT_R32G32B32_SFLOAT;
		vertexElement.offset = 0;
		vertexElement.stride = 3 * sizeof(float);
		vertexElement.useInstanceIndex = false;
		pipelineInfo.vertexInputState.vertexElements.push_back(vertexElement);
		m_BoxPipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);
	}

	// sphere
	{
        m_SphereVertShader = std::make_shared<Shader>("gizmo/GizmoSphere", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
        m_SphereFragShader = std::make_shared<Shader>("gizmo/GizmoSphere", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
        RHIGraphicsPipelineInfo pipelineInfo = {};
        pipelineInfo.vertexShader = m_SphereVertShader;
        pipelineInfo.fragmentShader = m_SphereFragShader;
        pipelineInfo.primitiveType = PRIMITIVE_TYPE_TRIANGLE_LIST;
        pipelineInfo.rasterizerState = { FILL_MODE_WIREFRAME, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
        pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
        pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.blendState.renderTargets[0].enable = false;
        pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
        pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
        VertexElement vertexElement = {};
        vertexElement.streamIndex = 0;
        vertexElement.attributeIndex = 0;
        vertexElement.format = FORMAT_R32G32B32_SFLOAT;	
        vertexElement.offset = 0;
        vertexElement.stride = 3 * sizeof(float);
        vertexElement.useInstanceIndex = false;
        pipelineInfo.vertexInputState.vertexElements.push_back(vertexElement);
        m_SpherePipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);
	}
	
	// line
	{
        m_LineVertShader = std::make_shared<Shader>("gizmo/GizmoLine", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
        m_LineFragShader = std::make_shared<Shader>("gizmo/GizmoLine", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
		m_LineGeomShader = std::make_shared<Shader>("gizmo/GizmoLine", SHADER_FREQUENCY_GEOMETRY)->GetRHIShader();
        RHIGraphicsPipelineInfo pipelineInfo = {};
        pipelineInfo.vertexShader = m_LineVertShader;
        pipelineInfo.fragmentShader = m_LineFragShader;
        pipelineInfo.geometryShader = m_LineGeomShader;
        pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
        pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
        pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.blendState.renderTargets[0].enable = false;
        pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
        pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_POINT_LIST;
		m_LinePipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);
	}

	// billboard
	{
		m_BillboardVertShader = std::make_shared<Shader>("gizmo/GizmoBillboard", SHADER_FREQUENCY_VERTEX)->GetRHIShader();
        m_BillboardFragShader = std::make_shared<Shader>("gizmo/GizmoBillboard", SHADER_FREQUENCY_FRAGMENT)->GetRHIShader();
		m_BillboardGeomShader = std::make_shared<Shader>("gizmo/GizmoBillboard", SHADER_FREQUENCY_GEOMETRY)->GetRHIShader();

		RHIGraphicsPipelineInfo pipelineInfo = {};
        pipelineInfo.vertexShader = m_BillboardVertShader;
        pipelineInfo.geometryShader = m_BillboardGeomShader;
        pipelineInfo.fragmentShader = m_BillboardFragShader;
		pipelineInfo.rasterizerState = { FILL_MODE_SOLID, CULL_MODE_NONE, DEPTH_CLIP, 0.0f, 0.0f };
		pipelineInfo.depthStencilState = { COMPARE_FUNCTION_LESS_EQUAL, true, true };
		pipelineInfo.rootSignature = m_RootSignature;
		pipelineInfo.blendState.renderTargets[0].enable = false;
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R32G32B32A32_SFLOAT;
		pipelineInfo.depthStencilAttachmentFormat = FORMAT_D32_SFLOAT;
		pipelineInfo.primitiveType = PRIMITIVE_TYPE_POINT_LIST; // 顶点只发发射点

		m_BillboardPipeline = APP_DYNAMICRHI->CreateGraphicsPipeline(pipelineInfo);
	}


	{
		ModelSpec m_ModelSpec;
        m_ModelSpec.uploadGPU = true;
		cubeWire = std::make_shared<Model>(APP_MODEL_PATH + "Basic/cube_wire.obj", m_ModelSpec);
		cubeWire->OnLoadAsset();
		command[0].indexCount = cubeWire->GetIndexBuffer(0)->IndexNum();
		command[0].instanceCount = 0;
		command[0].firstIndex = 0;
		command[0].vertexOffset = 0;
		command[0].firstInstance = 0;

		sphereWire = std::make_shared<Model>(APP_MODEL_PATH + "Basic/sphere.obj", m_ModelSpec);
		sphereWire->OnLoadAsset();
		command[1].indexCount = sphereWire->GetIndexBuffer(0)->IndexNum();
		command[1].instanceCount = 0;
		command[1].firstIndex = 0;
		command[1].vertexOffset = 0;
		command[1].firstInstance = 0;

		command[2].indexCount = 1;
		command[2].instanceCount = 0;
		command[2].firstIndex = 0;
		command[2].vertexOffset = 0;
		command[2].firstInstance = 0;

		command[3].indexCount = 1;
		command[3].instanceCount = 0;
		command[3].firstIndex = 0;
		command[3].vertexOffset = 0;
		command[3].firstInstance = 0;
	}
}

void GameEngine::GizmoPass::Build(RDGBuilder& builder)
{
	// 刷新Buffer
	RENDER_RESOURCEMANAGER->SetGizmoDataCommand(&command[0], 4);

    RDGBufferHandle dataBuffer = builder.CreateBuffer("Gizmo Data")
        .Import(RENDER_RESOURCEMANAGER->GetGizmoDataBuffer(), RESOURCE_STATE_UNDEFINED)
        .Finish();

	// 通过设置信息装填各种光源信息到GizmoDrawData
	builder.CreateComputePass("Gizmo_Init")
		.RootSignature(m_RootSignature)
		.Execute([&](RDGPassContext context) { 
			 RHICommandListRef command = context.command;
			 command->SetComputePipeline(m_GizmoInitPipeline);
			 command->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
			 command->Dispatch(1, 1, 1);
		})
        .OutputIndirectDraw(dataBuffer)   // 必须把dataBuffer添加屏障，否则后续IndirectDraw会出错
		.Finish();



	RDGTextureHandle viewport = builder.GetTexture("RenderRes");
	RDGTextureHandle depth = builder.GetTexture("Depth");


    // ------------------------------------------------------------
    // BoxPass
    // ------------------------------------------------------------
    builder.CreateRenderPass("Gizmo_BoxPass")
        .Color(0, viewport, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, { 0,0,0,0 })
        .DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)
        .Execute([&](RDGPassContext context)
            {
                auto [w, h] = APP_WINDOWSIZE;

                RHICommandListRef cmd = context.command;
                cmd->SetGraphicsPipeline(m_BoxPipeline);
                cmd->SetViewport({ 0,0 }, { w,h });
                cmd->SetScissor({ 0,0 }, { w,h });
                cmd->SetLineWidth(1.0f);
                cmd->SetDepthBias(0, 0, 0);
                cmd->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
                cmd->BindVertexBuffer(cubeWire->GetVertexBuffer(0)->positionBuffer, 0, 0);
                cmd->BindIndexBuffer(cubeWire->GetIndexBuffer(0)->buffer, 0);
                cmd->DrawIndexedIndirect(RENDER_RESOURCEMANAGER->GetGizmoDataBuffer(),
                    0,
                    1);
            })
        .OutputRead(viewport)
        .Finish();


    // ------------------------------------------------------------
    // SpherePass
    // ------------------------------------------------------------
    builder.CreateRenderPass("Gizmo_SpherePass")
        .Color(0, viewport, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, { 0,0,0,0 })
        .DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)
        .Execute([&](RDGPassContext context)
            {
                auto [w, h] = APP_WINDOWSIZE;

                RHICommandListRef cmd = context.command;
                cmd->SetGraphicsPipeline(m_SpherePipeline);
                cmd->SetViewport({ 0,0 }, { w,h });
                cmd->SetScissor({ 0,0 }, { w,h });
                cmd->SetLineWidth(1.0f);
                cmd->SetDepthBias(0, 0, 0);
                cmd->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);
                cmd->BindVertexBuffer(sphereWire->GetVertexBuffer(0)->positionBuffer, 0, 0);
                cmd->BindIndexBuffer(sphereWire->GetIndexBuffer(0)->buffer, 0);

                cmd->DrawIndexedIndirect(RENDER_RESOURCEMANAGER->GetGizmoDataBuffer(),
                    sizeof(RHIIndexedIndirectCommand),
                    1);
            })
        .OutputRead(viewport)
        .Finish();


    // ------------------------------------------------------------
    // LinePass
    // ------------------------------------------------------------
    builder.CreateRenderPass("Gizmo_LinePass")
        .Color(0, viewport, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, { 0,0,0,0 })
        .DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)
        .Execute([&](RDGPassContext context)
            {
                auto [w, h] = APP_WINDOWSIZE;

                RHICommandListRef cmd = context.command;
                cmd->SetGraphicsPipeline(m_LinePipeline);
                cmd->SetViewport({ 0,0 }, { w,h });
                cmd->SetScissor({ 0,0 }, { w,h });
                cmd->SetLineWidth(1.0f);
                cmd->SetDepthBias(0, 0, 0);
                cmd->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);

                cmd->DrawIndexedIndirect(RENDER_RESOURCEMANAGER->GetGizmoDataBuffer(),
                    2 * sizeof(RHIIndexedIndirectCommand),
                    1);
            })
        .OutputRead(viewport)
        .Finish();


    // ------------------------------------------------------------
    // BillboardPass
    // ------------------------------------------------------------
    builder.CreateRenderPass("Gizmo_BillboardPass")
        .Color(0, viewport, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, { 0,0,0,0 })
        .DepthStencil(depth, ATTACHMENT_LOAD_OP_LOAD, ATTACHMENT_STORE_OP_STORE, 1.0f, 0)
        .Execute([&](RDGPassContext context)
            {
                auto [w, h] = APP_WINDOWSIZE;

                RHICommandListRef cmd = context.command;
                cmd->SetGraphicsPipeline(m_BillboardPipeline);
                cmd->SetViewport({ 0,0 }, { w,h });
                cmd->SetScissor({ 0,0 }, { w,h });
                cmd->SetLineWidth(1.0f);
                cmd->SetDepthBias(0, 0, 0);
                cmd->BindDescriptorSet(RENDER_RESOURCEMANAGER->GetGlobalResourcePerFrameDescriptorSet(), 0);

                cmd->DrawIndexedIndirect(RENDER_RESOURCEMANAGER->GetGizmoDataBuffer(),
                    3 * sizeof(RHIIndexedIndirectCommand),
                    1);
            })
        .OutputRead(viewport)
        .Finish();


}
