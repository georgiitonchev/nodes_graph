#include "../commands.h"
#include "../node_connection.h"

#include <map>
#include <string>

class DeleteConnectionCommand : public _Command {
private:
	NodeConnection* _connection;
	std::map<std::string, NodeConnection*>* _map;

public:
	~DeleteConnectionCommand() {
		if (_state == Executed)
			delete _connection;
	}

	DeleteConnectionCommand(NodeConnection* connection, std::map<std::string, NodeConnection*>* map) :
		_Command("Delete Connection"),
		_connection(connection), _map(map)
	{
	}

protected:
	void _Execute() override {
		_map->erase(_connection->GetId());
		_connection->GetFrom()->RemoveConnectionFrom();
		_connection->GetTo()->RemoveConnectionTo();
	}

	void _Undo() override {
		_map->emplace(_connection->GetId(), _connection);
		_connection->GetFrom()->AddConnectionFrom();
		_connection->GetTo()->AddConnectionTo();
	}

	void _Redo() override {
		_map->erase(_connection->GetId());
		_connection->GetFrom()->RemoveConnectionFrom();
		_connection->GetTo()->RemoveConnectionTo();
	}
};