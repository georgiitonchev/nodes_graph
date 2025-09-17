#pragma once

#include <stack>
#include <vector>

enum CommandState {
	Created,
	Executed,
	Reverted
};

class _Command {
protected:
	virtual void _Execute() = 0;
	virtual void _Undo() = 0;
	virtual void _Redo() = 0;

	const char* _label;
	CommandState _state;

public:
	_Command(const char* label) :
		_label(label),
		_state(Created)
	{
	}

	virtual ~_Command() = default;

	void Execute() {
		_Execute();
		_state = Executed;
	}

	void Undo() {
		_Undo();
		_state = Reverted;
	}

	void Redo() {
		_Redo();
		_state = Executed;
	}

	const char* GetLabel() const {
		return _label;
	}

	const CommandState GetState() const {
		return _state;
	}
};

class CommandCluster : public _Command {
private:
	std::vector<_Command*> _commands;

	void _Execute() override {
		for (const auto& command : _commands) {
			command->Execute();
		}
	}

	void _Undo() override {
		for (const auto& command : _commands) {
			command->Undo();
		}
	}

	void _Redo() override {
		for (const auto& command : _commands) {
			command->Redo();
		}
	}

public:
	CommandCluster(const char* label) : _Command(label) {}

	~CommandCluster() {
		for (const auto& command : _commands) {
			delete command;
		}
	}

	void Add(_Command* command) {
		_commands.push_back(command);
	}
};

class Commands {
private:
	std::vector<_Command*> _undoStack;
	std::vector<_Command*> _redoStack;

	int _commandIndex = 0;

public:
	std::vector<_Command*>& GetUndoStack()
	{
		return _undoStack;
	}

	std::vector<_Command*>& GetRedoStack()
	{
		return _redoStack;
	}

	void Execute(_Command* command) {
		command->Execute();
		_undoStack.push_back(command);

		for (auto& it : _redoStack) {
			delete it;
		}

		_redoStack.clear();

		_commandIndex++;
	}

	void Undo() {
		if (!HasUndo()) return;

		auto undoCommand = _undoStack.back();
		undoCommand->Undo();

		_redoStack.push_back(undoCommand);
		_undoStack.erase(_undoStack.end() - 1);

		_commandIndex--;
	}

	void Redo() {
		if (!HasRedo()) return;

		auto redoCommand = _redoStack.back();
		redoCommand->Redo();

		_undoStack.push_back(redoCommand);
		_redoStack.erase(_redoStack.end() - 1);

		_commandIndex++;
	}

	bool HasUndo() {
		return _undoStack.size() > 0;
	}

	bool HasRedo() {
		return _redoStack.size() > 0;
	}

	const int CommandIndex() const {
		return _commandIndex;
	}

	void Clear() {
		for (const auto& command : _redoStack) {
			delete command;
		}

		for (const auto& command : _undoStack) {
			delete command;
		}
	}
};