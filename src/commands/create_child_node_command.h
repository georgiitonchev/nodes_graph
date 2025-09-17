#pragma once
#include "../commands.h"
#include "../node.h"

class CreateChildNodeCommand : public _Command {
private:
	Node* _node;
	std::vector<Node*>* _vector;

public:
	~CreateChildNodeCommand() {
		if (_state == Reverted)
			delete _node;
	}

	CreateChildNodeCommand(Node* node, std::vector<Node*>* vector) :
		_Command("Create Child Node"),
		_node(node),
		_vector(vector)
	{
	}

protected:
	void _Execute() override {
		_vector->emplace_back(_node);
	}

	void _Undo() override {
		_vector->erase(_vector->end() - 1);
	}

	void _Redo() override {
		_vector->emplace_back(_node);
	}
};