#pragma once
#include "../commands.h"
#include "../node.h"

class CreateNodeCommand : public _Command {
private:
	Node* _node;
	std::map<std::string, Node*>* _map;

public:
	~CreateNodeCommand() {
		if (_state == Reverted || _state == Created)
			delete _node;
	}

	CreateNodeCommand(Node* node, std::map<std::string, Node*>* map) :
		_Command("Create Node"),
		_node(node),
		_map(map)
	{
	}

protected:
	void _Execute() override {
		_map->emplace(_node->GetId(), _node);
	}

	void _Undo() override {
		_map->erase(_node->GetId());
	}

	void _Redo() override {
		_map->emplace(_node->GetId(), _node);
	}
};