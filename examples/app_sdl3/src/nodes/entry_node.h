#pragma once
#include "node.h"

class EntryNode : public Node
{
private:
	void _Init() override
	{
		AddSlot(SlotPosition::Left, false, true);
		AddSlot(SlotPosition::Top, false, true);
		AddSlot(SlotPosition::Right, false, true);
		AddSlot(SlotPosition::Bottom, false, true);
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
		return new EntryNode();
	}
};