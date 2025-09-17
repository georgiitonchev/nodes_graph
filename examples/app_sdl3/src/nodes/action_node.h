#pragma once
#include "node.h"
#include "../input.h"

class ActionNode : public Node
{
private:
	std::string _value;

	void _Init() override
	{
		_outputRequired = false;
		_colorOutline = IM_COL32(147, 47, 103, 255);

		AddSlot(SlotPosition::Left, true, true);
		AddSlot(SlotPosition::Top, true, true);
		AddSlot(SlotPosition::Right, true, true);
		AddSlot(SlotPosition::Bottom, true, true);
	}

	void _Draw(ImDrawList* drawList) override
	{
		Input::Text("Value", &_value);
	}

	void _ToJson(nlohmann::json& j) override
	{
		j["value"] = _value;
	}

	void _FromJson(const nlohmann::json& j) override
	{
		j.at("value").get_to(_value);
	}

	bool _Validate() override
	{
		if (_value.empty())
		{
			SetValidationMessage("Value cannot be empty.");
			return false;
		}

		return true;
	}

	Node* _Clone() override
	{
		auto clone = new ActionNode();
		clone->_value = _value;
		return clone;
	}
};