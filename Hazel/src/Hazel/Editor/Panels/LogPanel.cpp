#include "hzpch.h"
#include "LogPanel.h"
#include <imgui.h>

namespace GameEngine {

	ImGuiLogPanel::ImGuiLogPanel()
		: m_ShowTrace(false)
		, m_ShowInfo(false)
		, m_ShowWarn(false)
		, m_ShowError(true)
		, m_ScrollToBottom(true)
	{
	}

	bool ImGuiLogPanel::ShouldShow(GameEngine::LogLevel level) const
	{
		switch (level)
		{
		case LogLevel::trace: return m_ShowTrace;
		case LogLevel::info: return m_ShowInfo;
		case LogLevel::warn: return m_ShowWarn;
		case LogLevel::error: return m_ShowError;
		default: return true;
		}
	}

	void ImGuiLogPanel::OnImGuiRender()
	{
		if (!isOpen)
			return;

		ImGui::Begin("Log Panel", &isOpen);

		// 日志级别过滤
		ImGui::Checkbox("trace", &m_ShowTrace); ImGui::SameLine();
		ImGui::Checkbox("info", &m_ShowInfo); ImGui::SameLine();
		ImGui::Checkbox("warn", &m_ShowWarn); ImGui::SameLine();
		ImGui::Checkbox("error", &m_ShowError); ImGui::SameLine();

		ImGui::Separator();

		if (ImGui::Button("Clear"))
		{
			std::lock_guard<std::mutex> lock(GameEngine::Log::s_LogMutex);
			GameEngine::Log::s_LogCache.clear();
			m_ScrollToBottom = true; // 清空后滚动到底
		}

		ImGui::BeginChild("LogRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

		const auto& logs = GameEngine::Log::GetCache();
		for (const auto& log : logs)
		{
			if (!ShouldShow(log.Level))
				continue;

			ImVec4 color;
			switch (log.Level)
			{
			case LogLevel::trace:    color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); break; // 灰色
			case LogLevel::info:     color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break; // 绿色
			case LogLevel::warn:     color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break; // 黄色
			case LogLevel::error:    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break; // 红色
			default:                 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break; // 默认白色
			}

			ImGui::PushStyleColor(ImGuiCol_Text, color);

			// 判断是否是 Vulkan Validation Error
			if (log.Level == LogLevel::error && log.Message.find("[Validation]") != std::string::npos)
			{
				ImGui::TextUnformatted("Vulkan Validation LOG_ERROR"); // 替换显示内容
			}
			else
			{
				ImGui::TextUnformatted(log.Message.c_str()); // 正常显示
			}

			ImGui::PopStyleColor();
		}


		// 自动滚动到底
		if (m_ScrollToBottom || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		{
			ImGui::SetScrollHereY(1.0f);
			m_ScrollToBottom = false;
		}

		ImGui::EndChild();
		ImGui::End();
	}

} // namespace GameEngine
