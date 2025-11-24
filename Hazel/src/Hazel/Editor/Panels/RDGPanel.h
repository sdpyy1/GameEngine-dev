#pragma once
#include "EditorPanel.h"
#include "imgui_node_editor.h"
namespace ed = ax::NodeEditor;

namespace GameEngine {

	class RDGPanel : public EditorPanel
	{
	public:
		RDGPanel();
		virtual ~RDGPanel() = default;

		virtual void OnImGuiRender() override;

	private:
		ed::EditorContext* m_Context = nullptr;
	};

} // namespace GameEngine


