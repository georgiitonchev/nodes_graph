#pragma once
#include "node.h"

class ExitNode : public Node
{
private:
	void _Init() override
	{
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
		return new ExitNode();
	}
};