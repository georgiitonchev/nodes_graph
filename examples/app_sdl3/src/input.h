#pragma once

#include <string>

#include "imgui.h"
#include "nodes_graph.h"

#include "commands/edit_value_command.h"

class Input {
private:
	inline static std::string _previousStringValue;
	inline static float _previousFloatValue;

public:
	static inline void Text(const char* label, std::string* str, float width = 192_dpi, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr)
	{
		ImGui::SetNextItemWidth(width);
		ImGui::InputText(label, str, flags, callback, user_data);

		if (ImGui::IsItemActivated())
			_previousStringValue = *str;

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			if (str->compare(_previousStringValue) != 0)
				NodesGraph::GetCurrent()->Execute(new EditValueCommand<std::string>(str, _previousStringValue));
		}
	}

	static inline void Float(const char* label, float* v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
	{
		auto previousValue = *v;
		ImGui::SetNextItemWidth(192_dpi);
		ImGui::InputFloat(label, v, step, step_fast, format, flags);

		if (ImGui::IsItemActivated()) {
			_previousFloatValue = *v;
		}

		if (ImGui::IsItemDeactivatedAfterEdit()) {
			if (_previousFloatValue != *v)
				NodesGraph::GetCurrent()->Execute(new EditValueCommand<float>(v, _previousFloatValue));
		}
		else if (ImGui::IsItemEdited() && ImGui::IsItemActivated() && previousValue != *v)
		{
			if (previousValue != *v)
				NodesGraph::GetCurrent()->Execute(new EditValueCommand<float>(v, previousValue));
		}
	}
};