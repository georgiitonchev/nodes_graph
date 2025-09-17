#pragma once
#include "../commands.h"
#include "../node.h"

class DeleteChildNodeCommand : public _Command {
private:
	Node* _node;
	std::vector<Node*>* _vector;
	size_t _index;

public:
	~DeleteChildNodeCommand() {
		if (_state == Executed)
			delete _node;
	}

	DeleteChildNodeCommand(Node* node, std::vector<Node*>* vector) :
		_Command("Delete Child Node"),
		_index(0),
		_node(node),
		_vector(vector)
	{
	}

protected:
	void _Execute() override {
		auto it = std::find(_vector->begin(), _vector->end(), _node);
		_index = std::distance(_vector->begin(), it);
		_vector->erase(it);
	}

	void _Undo() override {
		_vector->insert(_vector->begin() + _index, _node);
	}

	void _Redo() override {
		_vector->erase(_vector->begin() + _index);
	}
};