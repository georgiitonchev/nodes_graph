#pragma once
#include "../commands.h"

template<typename T>
class EditValueCommand : public _Command {
private:
  T _valuePrevious;
  T _valueCurrent;
  T* _valuePtr;

public:
  EditValueCommand(T* valuePtr, T valuePrevious) : _Command("Edit Value")
  {
    _valuePrevious = valuePrevious;
    _valueCurrent = *valuePtr;
    _valuePtr = valuePtr;
  }

protected:
	void _Execute() override {
		*_valuePtr = _valueCurrent;
	}

	void _Undo() override {
		*_valuePtr = _valuePrevious;
	}

	void _Redo() override {
		*_valuePtr = _valueCurrent;
	}
};