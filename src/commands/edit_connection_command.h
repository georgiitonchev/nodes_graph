#include "../commands.h"
#include "../node_connection.h"

#include <map>
#include <string>

class EditConnectionCommand : public _Command {
private:
	NodeConnection* _connection;
	NodeSlot* _fromPrev, * _fromCurr;
	NodeSlot* _toPrev, * _toCurr;

public:
	EditConnectionCommand(NodeConnection* connection, NodeSlot* from, NodeSlot* to) :
		_Command("Edit Connection"),
		_connection(connection),
		_fromCurr(from),
		_toCurr(to)
	{
		_fromPrev = connection->GetFrom();
		_toPrev = connection->GetTo();
	}

protected:
	void _Execute() override {
		if (_fromPrev != _fromCurr) {
			_connection->SetFrom(_fromCurr);
			_fromPrev->RemoveConnectionFrom();
			_fromCurr->AddConnectionFrom();
		}

		if (_toPrev != _toCurr) {
			_connection->SetTo(_toCurr);
			_toPrev->RemoveConnectionTo();
			_toCurr->AddConnectionTo();
		}
	}

	void _Undo() override {
		if (_fromPrev != _fromCurr) {
			_connection->SetFrom(_fromPrev);
			_fromCurr->RemoveConnectionFrom();
			_fromPrev->AddConnectionFrom();
		}

		if (_toPrev != _toCurr) {
			_connection->SetTo(_toPrev);
			_toCurr->RemoveConnectionTo();
			_toPrev->AddConnectionTo();
		}
	}

	void _Redo() override {
		if (_fromPrev != _fromCurr) {
			_connection->SetFrom(_fromCurr);
			_fromPrev->RemoveConnectionFrom();
			_fromCurr->AddConnectionFrom();
		}

		if (_toPrev != _toCurr) {
			_connection->SetTo(_toCurr);
			_toPrev->RemoveConnectionTo();
			_toCurr->AddConnectionTo();
		}
	}
};