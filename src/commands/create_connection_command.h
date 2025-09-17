#include "../commands.h"
#include "../node_connection.h"

#include <map>
#include <string>

class CreateConnectionCommand : public _Command {
private:
	NodeConnection* _connection;
	std::map<std::string, NodeConnection*>* _map;

public:
	~CreateConnectionCommand() {
		if (_state == Reverted || _state == Created)
			delete _connection;
	}

	CreateConnectionCommand(NodeConnection* connection, std::map<std::string, NodeConnection*>* map) :
		_Command("Create Connection"),
		_connection(connection), _map(map)
	{
	}

protected:
	void _Execute() override {
		_map->emplace(_connection->GetId(), _connection);
		_connection->GetFrom()->AddConnectionFrom();
		_connection->GetTo()->AddConnectionTo();
	}

	void _Undo() override {
		_map->erase(_connection->GetId());
		_connection->GetFrom()->RemoveConnectionFrom();
		_connection->GetTo()->RemoveConnectionTo();
	}

	void _Redo() override {
		_map->emplace(_connection->GetId(), _connection);
		_connection->GetFrom()->AddConnectionFrom();
		_connection->GetTo()->AddConnectionTo();
	}
};