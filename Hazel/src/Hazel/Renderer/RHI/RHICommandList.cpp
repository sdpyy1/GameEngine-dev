#include "hzpch.h"
#include "RHICommandList.h"
#include "RHI.h"
namespace GameEngine {
	void RHICommandBeginCommand::Execute(RHICommandContextRef context)
	{
		context->BeginCommand();
	}

	void RHICommandEndCommand::Execute(RHICommandContextRef context)
	{
		context->EndCommand();
	}

	void RHICommandList::BeginCommand()
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BeginCommand();
		else ADD_COMMAND(BeginCommand);
	}

	void RHICommandList::EndCommand()
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->EndCommand();
		else ADD_COMMAND(EndCommand);
	}

	void RHICommandList::Execute(RHIFenceRef fence, RHISemaphoreRef waitSemaphore, RHISemaphoreRef signalSemaphore)
	{
		if (!info.byPass)
		{
			// LOG_DEBUG("Recording command list in delay mode.");
			for (int32_t i = 0; i < commands.size(); i++)
			{
				commands[i]->Execute(info.context);
				delete commands[i];
			}
			commands.clear();
		}

		info.context->Execute(fence, waitSemaphore, signalSemaphore);
	}

	void RHICommandList::BufferBarrier(const RHIBufferBarrier& barrier)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BufferBarrier(barrier);
		else ADD_COMMAND(BufferBarrier, barrier);
	}

	void RHICommandList::BeginRenderPass(RHIRenderPassRef renderPass)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BeginRenderPass(renderPass);
		else ADD_COMMAND(BeginRenderPass, renderPass);
	}

	void RHICommandList::EndRenderPass()
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->EndRenderPass();
		else ADD_COMMAND(EndRenderPass);
	}

	void RHICommandList::CopyTexture(RHITextureRef src, TextureSubresourceLayers srcSubresource, RHITextureRef dst, TextureSubresourceLayers dstSubresource)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->CopyTexture(src, srcSubresource, dst, dstSubresource);
		else ADD_COMMAND(CopyTexture, src, srcSubresource, dst, dstSubresource);
	}

	void RHICommandList::GenerateMips(RHITextureRef src)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->GenerateMips(src);
		else ADD_COMMAND(GenerateMips, src);
	}

	void RHICommandList::TextureBarrier(const RHITextureBarrier& barrier)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->TextureBarrier(barrier);
		else ADD_COMMAND(TextureBarrier, barrier);
	}

	void RHICommandImmediateGenerateMips::Execute(RHICommandContextImmediateRef context)
	{
		context->GenerateMips(src);
	}

	void RHICommandListImmediate::GenerateMips(RHITextureRef src)
	{
		ADD_COMMAND_IMMEDIATE(GenerateMips, src);
	}

	void RHICommandListImmediate::TextureBarrier(const RHITextureBarrier& barrier)
	{
		ADD_COMMAND_IMMEDIATE(TextureBarrier, barrier);
	}

	void RHICommandListImmediate::UploadImGuiFonts()
	{
		ADD_COMMAND_IMMEDIATE(UploadImGuiFonts);
	}

	void RHICommandListImmediate::Flush()
	{
		// LOG_DEBUG("RHICommandListImmediate Flushed.");
		for (int32_t i = 0; i < commands.size(); i++)
		{
			commands[i]->Execute(info.context);
			delete commands[i];
		}
		commands.clear();

		info.context->Flush();
	}

	void RHICommandImmediateTextureBarrier::Execute(RHICommandContextImmediateRef context)
	{
		context->TextureBarrier(barrier);
	}

	void RHICommandBufferBarrier::Execute(RHICommandContextRef context)
	{
		context->BufferBarrier(barrier);
	}

	void RHICommandGenerateMips::Execute(RHICommandContextRef context)
	{
		context->GenerateMips(src);
	}

	void RHICommandBeginRenderPass::Execute(RHICommandContextRef context)
	{
		context->BeginRenderPass(renderPass);
	}

	void RHICommandEndRenderPass::Execute(RHICommandContextRef context)
	{
		context->EndRenderPass();
	}

	void RHICommandCopyTexture::Execute(RHICommandContextRef context)
	{
		context->CopyTexture(src, srcSubresource, dst, dstSubresource);
	}

	void RHICommandTextureBarrier::Execute(RHICommandContextRef context)
	{
		context->TextureBarrier(barrier);
	}

	void RHICommandSetViewport::Execute(RHICommandContextRef context)
	{
		context->SetViewport(min, max);
	}

	void RHICommandSetScissor::Execute(RHICommandContextRef context)
	{
		context->SetScissor(min, max);
	}

	void RHICommandSetDepthBias::Execute(RHICommandContextRef context)
	{
		context->SetDepthBias(constantBias, slopeBias, clampBias);
	}

	void RHICommandSetLineWidth::Execute(RHICommandContextRef context)
	{
		context->SetLineWidth(width);
	}


	void RHICommandSetGraphicsPipeline::Execute(RHICommandContextRef context)
	{
		context->SetGraphicsPipeline(graphicsPipeline);
	}

	void RHICommandSetComputePipeline::Execute(RHICommandContextRef context)
	{
		context->SetComputePipeline(computePipeline);
	}

	void RHICommandPushConstants::Execute(RHICommandContextRef context)
	{
		context->PushConstants(data, size, frequency);
	}

	void RHICommandBindDescriptorSet::Execute(RHICommandContextRef context)
	{
		context->BindDescriptorSet(descriptor, set);
	}

	void RHICommandBindVertexBuffer::Execute(RHICommandContextRef context)
	{
		context->BindVertexBuffer(vertexBuffer, streamIndex, offset);
	}

	void RHICommandBindIndexBuffer::Execute(RHICommandContextRef context)
	{
		context->BindIndexBuffer(indexBuffer, offset);
	}

	void RHICommandDispatch::Execute(RHICommandContextRef context)
	{
		context->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void RHICommandDispatchIndirect::Execute(RHICommandContextRef context)
	{
		context->DispatchIndirect(argumentBuffer, argumentOffset);
	}

	void RHICommandTraceRays::Execute(RHICommandContextRef context)
	{
		context->TraceRays(groupCountX, groupCountY, groupCountZ);
	}

	void RHICommandDraw::Execute(RHICommandContextRef context)
	{
		context->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void RHICommandDrawIndexed::Execute(RHICommandContextRef context)
	{
		context->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void RHICommandDrawIndirect::Execute(RHICommandContextRef context)
	{
		context->DrawIndirect(argumentBuffer, offset, drawCount);
	}

	void RHICommandDrawIndexedIndirect::Execute(RHICommandContextRef context)
	{
		context->DrawIndexedIndirect(argumentBuffer, offset, drawCount);
	}
	void RHICommandList::SetViewport(Offset2D min, Offset2D max)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->SetViewport(min, max);
		else ADD_COMMAND(SetViewport, min, max);
	}

	void RHICommandList::SetScissor(Offset2D min, Offset2D max)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->SetScissor(min, max);
		else ADD_COMMAND(SetScissor, min, max);
	}

	void RHICommandList::SetDepthBias(float constantBias, float slopeBias, float clampBias)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->SetDepthBias(constantBias, slopeBias, clampBias);
		else ADD_COMMAND(SetDepthBias, constantBias, slopeBias, clampBias);
	}

	void RHICommandList::SetLineWidth(float width)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->SetLineWidth(width);
		else ADD_COMMAND(SetLineWidth, width);
	}

	void RHICommandList::SetGraphicsPipeline(RHIGraphicsPipelineRef graphicsPipeline)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->SetGraphicsPipeline(graphicsPipeline);
		else ADD_COMMAND(SetGraphicsPipeline, graphicsPipeline);
	}

	void RHICommandList::SetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->SetRayTracingPipeline(rayTracingPipeline);
		else ADD_COMMAND(SetRayTracingPipeline, rayTracingPipeline);
	}

	void RHICommandList::SetComputePipeline(RHIComputePipelineRef computePipeline)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->SetComputePipeline(computePipeline);
		else ADD_COMMAND(SetComputePipeline, computePipeline);
	}

	/*void RHICommandList::SetRayTracingPipeline(RHIRayTracingPipelineRef rayTracingPipeline)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->SetRayTracingPipeline(rayTracingPipeline);
		else ADD_COMMAND(SetRayTracingPipeline, rayTracingPipeline);
	}*/

	void RHICommandList::PushConstants(void* data, uint16_t size, ShaderFrequency frequency)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->PushConstants(data, size, frequency);
		else ADD_COMMAND(PushConstants, data, size, frequency);
	}

	void RHICommandList::BindDescriptorSet(RHIDescriptorSetRef descriptor, uint32_t set)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BindDescriptorSet(descriptor, set);
		else ADD_COMMAND(BindDescriptorSet, descriptor, set);
	}

	void RHICommandList::BindVertexBuffer(RHIBufferRef vertexBuffer, uint32_t streamIndex, uint32_t offset)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BindVertexBuffer(vertexBuffer, streamIndex, offset);
		else ADD_COMMAND(BindVertexBuffer, vertexBuffer, streamIndex, offset);
	}

	void RHICommandList::BindIndexBuffer(RHIBufferRef indexBuffer, uint32_t offset)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->BindIndexBuffer(indexBuffer, offset);
		else ADD_COMMAND(BindIndexBuffer, indexBuffer, offset);
	}

	void RHICommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->Dispatch(groupCountX, groupCountY, groupCountZ);
		else ADD_COMMAND(Dispatch, groupCountX, groupCountY, groupCountZ);
	}

	void RHICommandList::DispatchIndirect(RHIBufferRef argumentBuffer, uint32_t argumentOffset)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->DispatchIndirect(argumentBuffer, argumentOffset);
		else ADD_COMMAND(DispatchIndirect, argumentBuffer, argumentOffset);
	}

	void RHICommandList::TraceRays(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->TraceRays(groupCountX, groupCountY, groupCountZ);
		else ADD_COMMAND(TraceRays, groupCountX, groupCountY, groupCountZ);
	}

	void RHICommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
		else ADD_COMMAND(Draw, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void RHICommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		else ADD_COMMAND(DrawIndexed, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void RHICommandList::DrawIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->DrawIndirect(argumentBuffer, offset, drawCount);
		else ADD_COMMAND(DrawIndirect, argumentBuffer, offset, drawCount);
	}

	void RHICommandList::DrawIndexedIndirect(RHIBufferRef argumentBuffer, uint32_t offset, uint32_t drawCount)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->DrawIndexedIndirect(argumentBuffer, offset, drawCount);
		else ADD_COMMAND(DrawIndexedIndirect, argumentBuffer, offset, drawCount);
	}
	void RHICommandList::ImGuiRenderDrawData()
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->ImGuiRenderDrawData();
		else ADD_COMMAND(ImGuiRenderDrawData);
	}

	void RHICommandList::PushLabel(const std::string& name, Color3 color)
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->PushLabel(name, color);
		else ADD_COMMAND(PushLabel, name, color);
	}

	void RHICommandList::PopLabel()
	{
		COMMANDLIST_DEBUG_OUTPUT();
		if (info.byPass) info.context->PopLabel();
		else ADD_COMMAND(PopLabel);
	}

	std::vector<GameEngine::RHIGPUTimeInfo> RHICommandList::GetGPUTime()
	{
		return info.context->GetGPUTime();
	}

	void RHICommandImGuiRenderDrawData::Execute(RHICommandContextRef context)
	{
		context->ImGuiRenderDrawData();
	}
	void RHICommandImmediateUploadImGuiFonts::Execute(RHICommandContextImmediateRef context)
	{
		context->ImGuiUploadFonts();
	}


	void RHICommandListImmediate::CopyBufferToTexture(RHIBufferRef src, uint64_t srcOffset, RHITextureRef dst, TextureSubresourceLayers dstSubresource)
	{
		ADD_COMMAND_IMMEDIATE(CopyBufferToTexture, src, srcOffset, dst, dstSubresource);
	}

	void RHICommandImmediateCopyBufferToTexture::Execute(RHICommandContextImmediateRef context)
	{
		context->CopyBufferToTexture(src, srcOffset, dst, dstSubresource);
	}

	void RHICommandPushLabel::Execute(RHICommandContextRef context)
	{
		context->PushLabel(name, color);
	}

	void RHICommandPopLabel::Execute(RHICommandContextRef context)
	{
		context->PopLabel();
	}

	void RHICommandSetRayTracingPipeline::Execute(RHICommandContextRef context)
	{
		context->SetRayTracingPipeline(rayTracingPipeline);
	}

}
