#pragma once
#include "node.h"
#include "../input.h"

class SpeechNode : public Node
{
private:
	std::string _target;
	std::string _text;

	void _Init() override
	{
		_colorOutline = IM_COL32(55, 149, 189, 255);

		AddSlot(SlotPosition::Left, true, true);
		AddSlot(SlotPosition::Top, true, true);
		AddSlot(SlotPosition::Right, true, true);
		AddSlot(SlotPosition::Bottom, true, true);
	}

	void _Draw(ImDrawList* drawList) override
	{
		Input::Text("Target", &_target);
		Input::Text("Text", &_text);
	}

	void _ToJson(nlohmann::json& j) override
	{
		j["target"] = _target;
		j["text"] = _text;
	}

	void _FromJson(const nlohmann::json& j) override
	{
		j.at("target").get_to(_target);
		j.at("text").get_to(_text);
	}

	bool _Validate() override
	{
		if (_target.empty())
		{
			SetValidationMessage("Target cannot be empty.");
			return false;
		}

		if (_text.empty())
		{
			SetValidationMessage("Text cannot be empty.");
			return false;
		}

		return true;
	}

	Node* _Clone() override
	{
		auto node = new SpeechNode();
		node->_target = _target;
		node->_text = _text;
		return node;
	}
};