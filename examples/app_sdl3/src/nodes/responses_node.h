#pragma once
#include "node.h"
#include "../input.h"

class ResponseNode : public Node {
private:
	std::string _text;

	void _Init() override
	{
		AddSlot(SlotPosition::Left, false, true);
		AddSlot(SlotPosition::Right, false, true);
	}

	void _Draw(ImDrawList* drawList) override
	{
		Input::Text("Text", &_text, 208_dpi);
	}

	void _ToJson(nlohmann::json& j) override
	{
		j["text"] = _text;
	}

	void _FromJson(const nlohmann::json& j) override
	{
		j.at("text").get_to(_text);
	}

	bool _Validate() override
	{
		if (_text.empty())
		{
			SetValidationMessage("Text cannot be empty.");
			return false;
		}
		return true;
	}

	Node* _Clone() override
	{
		auto clone = new ResponseNode();
		clone->_text = _text;
		return clone;
	}
};

class ResponsesNode : public GroupNode<ResponseNode> {
private:
	void _Init() override
	{
		_colorOutline = IM_COL32(255, 157, 35, 255);
		AddSlot(SlotPosition::Left, true, false);
		AddSlot(SlotPosition::Top, true, false);
		AddSlot(SlotPosition::Right, true, false);
		AddSlot(SlotPosition::Bottom, true, false);
	}

	void _Draw(ImDrawList* drawList) override
	{
	}

	void _ToJson(nlohmann::json& j) override
	{
	}

	void _FromJson(const nlohmann::json& j) override
	{
	}

	Node* _Clone() override
	{
		return new ResponsesNode();
	}
};