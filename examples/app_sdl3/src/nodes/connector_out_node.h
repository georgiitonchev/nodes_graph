#pragma once
#include "node.h"
#include "../input.h"

class ConnectorOutNode : public Node
{
private:
	std::string _value;

	void _Init() override
	{
		_colorOutline = ImColor(229, 56, 136);

		AddSlot(SlotPosition::Left, false, true);
		AddSlot(SlotPosition::Top, false, true);
		AddSlot(SlotPosition::Right, false, true);
		AddSlot(SlotPosition::Bottom, false, true);
	}

	void _Draw(ImDrawList* drawList) override
	{
		Input::Text("Id", &_value);
	}

	void _ToJson(nlohmann::json& j) override
	{
		j["value"] = _value;
	}

	void _FromJson(const nlohmann::json& j) override
	{
		j.at("value").get_to(_value);
	}

	Node* _Clone() override
	{
		auto clone = new ConnectorOutNode();
		clone->_value = _value;
		return clone;
	}

public:
	std::string GetValue() const { return _value; }
};