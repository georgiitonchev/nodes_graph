#pragma once
#include "../commands.h"
#include "../node.h"

class MoveNodeCommand : public _Command {
private:
	Node* _node;
	ImVec2 _positionFrom;
	ImVec2 _positionTo;

public:

	MoveNodeCommand(Node* node, ImVec2 positionTo) :
		_Command("Move Node"),
		_node(node),
		_positionTo(positionTo)
	{
		_positionFrom = node->GetRecordedPosition();
	}

protected:
	void _Execute() override {
		_node->SetPosition(_positionTo);
		_node->SetRecordedPosition(_positionTo);
	}

	void _Undo() override {
		_node->SetPosition(_positionFrom);
		_node->SetRecordedPosition(_positionFrom);
	}

	void _Redo() override {
		_node->SetPosition(_positionTo);
		_node->SetRecordedPosition(_positionTo);
	}
};