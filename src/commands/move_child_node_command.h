#pragma once
#include "../commands.h"
#include "../node.h"

class MoveChildNodeCommand : public _Command {
private:
	Node* _node;
	std::vector<Node*>& _vector;
	int _indexFrom;
	int _indexTo;

public:
	MoveChildNodeCommand(Node* node, std::vector<Node*>& vector, int indexFrom, int indexTo)
		: _Command("Move Child Node"), _node(node), _vector(vector), _indexFrom(indexFrom), _indexTo(indexTo) {
	}

protected:
	void _Execute() override {
		_vector.erase(_vector.begin() + _indexFrom);
		_vector.insert(_vector.begin() + _indexTo, _node);
	}

	void _Undo() override {
		_vector.erase(_vector.begin() + _indexTo);
		_vector.insert(_vector.begin() + _indexFrom, _node);
	}

	void _Redo() override {
		_vector.erase(_vector.begin() + _indexFrom);
		_vector.insert(_vector.begin() + _indexTo, _node);
	}
};