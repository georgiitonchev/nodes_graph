#pragma once
#include "../commands.h"
#include "../node.h"

class DeleteNodeCommand : public _Command {
private:
	Node* _node;
	std::map<std::string, Node*>* _map;

public:
	~DeleteNodeCommand() {
		if (_state == Executed)
			delete _node;
	}

	DeleteNodeCommand(Node* node, std::map<std::string, Node*>* map) :
		_Command("Delete Node"),
		_node(node),
		_map(map)
	{
	}

protected:
	void _Execute() override {
		_map->erase(_node->GetId());
	}

	void _Undo() override {
		_map->emplace(_node->GetId(), _node);
	}

	void _Redo() override {
		_map->erase(_node->GetId());
	}
};